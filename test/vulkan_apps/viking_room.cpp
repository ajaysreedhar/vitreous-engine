/**
 * viking-room.cpp - Vitreous Engine [test-vulkan-apps]
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2022 Ajay Sreedhar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================================================================
 */

#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <limits>
#include <cmath>
#include <algorithm>
#include "except/runtime.hpp"
#include "platform/logger.hpp"
#include "viking_room.hpp"

bool vtest::VikingRoom::s_isInitialised = false;
std::vector<std::string> vtest::VikingRoom::s_iExtensions {};
vtest::RGBAlpha vtest::VikingRoom::s_bgColor {
    0.0001f,
    0.0003f,
    0.0006f,
    1.0f
};

/**
 * Static utility function definitions.
 *
 * ========================================================================
 */

/**
 * Finds the GPU usability score after evaluating device properties.
 *
 * @param device An instance of VkPhysicalDevice
 * @return The calculated score
 */
unsigned int vtest::VikingRoom::findGPUScore_(VkPhysicalDevice device) {
    unsigned int score = 0;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    switch (properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score = score + 1000;
            break;

        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score = score + 750;
            break;

        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score = score + 500;
            break;

        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score = score + 250;
            break;

        default:
            break;
    }

    score = score + properties.limits.maxImageDimension2D;

    return score;
}

void vtest::VikingRoom::enumerateExtensions_() {
    uint32_t count = 0;
    auto result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Failed to query instance extensions.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    s_iExtensions.reserve(count);

    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());

    for (auto& item : extensions) {
        s_iExtensions.emplace_back(item.extensionName);
    }
}

vtest::SPIRVCode* vtest::VikingRoom::readSPIRVFile_(const std::string &abs_path) {
    std::ifstream spv_file(abs_path, std::ios::ate | std::ios::binary);

    if (!spv_file.is_open()) {
        throw vtrs::RuntimeError(abs_path, vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    auto file_size = spv_file.tellg();
    spv_file.seekg(0);

    auto spv_code = new vtest::SPIRVCode();
    spv_code->data = new char[file_size]();
    spv_code->size = file_size;

    spv_file.read(spv_code->data, file_size);
    spv_file.close();

    return spv_code;
}

/**
 * Private member function definitions.
 *
 * ========================================================================
 */
void vtest::VikingRoom::abortBootstrap_() {
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

/**
 * Initialises Vulkan instance.
 *
 * This method will set the m_instance member variable.
 */
void vtest::VikingRoom::initVulkan_(std::vector<const char*>& extensions) {
    VkApplicationInfo app_info {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vulkan Demo VikingRoom";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Vitreous Engine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> validation_layers({
        "VK_LAYER_KHRONOS_validation"
    });

    VkInstanceCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = extensions.size();
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();

    auto result = vkCreateInstance(&create_info, nullptr, &m_instance);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create Vulkan instance", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }
}

void vtest::VikingRoom::enumerateGPUExtensions_(VkPhysicalDevice device) {
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

    for (const auto& extension : extensions) {
        m_gExtensions.emplace_back(std::string(extension.extensionName));
    }
}

/**
 * Initialises physical GPU.
 *
 * This method will select a suitable GPU from the list of available
 * GPUs based on their score and set the m_gpu member variable.
 */
void vtest::VikingRoom::initGPU_() {
    uint32_t gpu_count = 0;

    auto result = vkEnumeratePhysicalDevices(m_instance, &gpu_count, nullptr);

    if (result != VK_SUCCESS || gpu_count == 0) {
        vkDestroyInstance(m_instance, nullptr);
        throw vtrs::RuntimeError("Unable to find any GPUs with Vulkan support.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    std::vector<VkPhysicalDevice> gpu_list(gpu_count);
    vkEnumeratePhysicalDevices(m_instance, &gpu_count, gpu_list.data());

    std::multimap<unsigned int, VkPhysicalDevice> candidates;

    for(VkPhysicalDevice& gpu : gpu_list) {
        unsigned int score = vtest::VikingRoom::findGPUScore_(gpu);
        candidates.insert(std::make_pair(score, gpu));
    }

    std::reverse_iterator selected = candidates.rbegin();

    if (selected->first == 0) {
        vkDestroyInstance(m_instance, nullptr);
        throw vtrs::RuntimeError("Unable to select a suitable GPU.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    this->enumerateGPUExtensions_(selected->second);
    m_gpu = selected->second;
}

vtest::QueueFamilyIndices vtest::VikingRoom::findQueueFamilies_() {
    uint32_t family_count = 0, family_index = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &family_count, nullptr);

    std::vector<VkQueueFamilyProperties> family_list(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &family_count, family_list.data());

    VkBool32 surfaceSupport;
    vtest::QueueFamilyIndices indices;

    for (auto& family : family_list) {
        vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, family_index, m_surface, &surfaceSupport);

        if (surfaceSupport >= 1){
            indices.surfaceIndex = family_index;
        }

        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsIndex = family_index;
        }

        if (indices.graphicsIndex.has_value() && indices.surfaceIndex.has_value()) {
            break;
        }

        family_index++;
    }

    return indices;
}

void vtest::VikingRoom::initDevice_(vtest::QueueFamilyIndices indices) {
    float queue_priority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    std::set<uint32_t> index_set = {indices.graphicsIndex.value(), indices.surfaceIndex.value()};

    for(uint32_t index : index_set) {
        VkDeviceQueueCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };

        queue_infos.push_back(create_info);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<const char*> extensions({
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    });

    VkDeviceCreateInfo device_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size()),
        .pQueueCreateInfos = queue_infos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

    auto result = vkCreateDevice(m_gpu, &device_info, nullptr, &m_device);

    if (result != VK_SUCCESS) {
        abortBootstrap_();
        throw vtrs::RuntimeError("Unable to create logical device.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    vkGetDeviceQueue(m_device, indices.graphicsIndex.value(), 0, &(m_queues.graphicsQueue));
    vkGetDeviceQueue(m_device, indices.surfaceIndex.value(), 0, &(m_queues.surfaceQueue));
}

vtest::SwapchainSupport vtest::VikingRoom::querySwapchainCapabilities_() {
    SwapchainSupport capabilities{};

    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu, m_surface, &(capabilities.capabilities));

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to query surface capabilities.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    uint32_t format_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &format_count, nullptr);

    if (result != VK_SUCCESS || format_count <= 0) {
        throw vtrs::RuntimeError("Unable to query surface formats.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    capabilities.surfaceFormats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &format_count, capabilities.surfaceFormats.data());

    uint32_t mode_count;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu, m_surface, &mode_count, nullptr);

    if (result != VK_SUCCESS || mode_count <= 0) {
        throw vtrs::RuntimeError("Unable to query surface present modes.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    capabilities.presentModes.resize(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu, m_surface, &mode_count, capabilities.presentModes.data());

    return capabilities;
}

void vtest::VikingRoom::createSwapchain_(QueueFamilyIndices indices) {
    SwapchainSupport support = querySwapchainCapabilities_();

    VkSwapchainCreateInfoKHR create_info {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface,
        .minImageCount = support.capabilities.minImageCount + 1,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    if (support.capabilities.maxImageCount > 0
        && create_info.minImageCount > support.capabilities.maxImageCount) {
        create_info.minImageCount = support.capabilities.maxImageCount;
    }

    create_info.imageFormat = support.surfaceFormats.front().format;
    create_info.imageColorSpace = support.surfaceFormats.front().colorSpace;

    for(const auto& current : support.surfaceFormats) {
        if (current.format == VK_FORMAT_B8G8R8A8_SRGB && current.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            create_info.imageFormat = current.format;
            create_info.imageColorSpace = current.colorSpace;
            break;
        }
    }

    if (support.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        create_info.imageExtent = support.capabilities.currentExtent;

    } else {
        int width = static_cast<uint32_t>(800);
        int height = static_cast<uint32_t>(600);

        create_info.imageExtent.width = std::clamp<uint32_t>(width, support.capabilities.minImageExtent.width, support.capabilities.maxImageExtent.width);
        create_info.imageExtent.height = std::clamp<uint32_t>(height, support.capabilities.minImageExtent.height, support.capabilities.maxImageExtent.height);
    }

    if (indices.graphicsIndex != indices.surfaceIndex) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;

        uint32_t pair[] = {indices.graphicsIndex.value(), indices.surfaceIndex.value()};
        create_info.pQueueFamilyIndices = pair;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    for(const auto& mode : support.presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            create_info.presentMode = mode;
            break;
        }
    }

    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    auto result = vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create swapchain.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    uint32_t image_count;
    result = vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);

    if (result != VK_SUCCESS || image_count == 0) {
        throw vtrs::RuntimeError("Unable to populate swapchain images.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    m_swapImages.resize(image_count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_swapImages.data());

    m_imageFormat = create_info.imageFormat;
    m_extend2D.width = create_info.imageExtent.width;
    m_extend2D.height = create_info.imageExtent.height;
}

void vtest::VikingRoom::createImageViews_() {
    m_swapImageViews.resize(m_swapImages.size());

    VkComponentMapping components {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };

    VkImageSubresourceRange range {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        1,
        0,
        1
    };

    int counter = 0;

    for(size_t index = 0; index < m_swapImages.size(); index++) {
        VkImageViewCreateInfo view_info {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_swapImages[index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_imageFormat,
            .components = components,
            .subresourceRange = range
        };

        if (vkCreateImageView(m_device, &view_info, nullptr, &(m_swapImageViews.at(index))) == VK_SUCCESS) {
            counter++;
        }
    }

    if (counter < m_swapImages.size()) {
        throw vtrs::RuntimeError("Unable to populate image views for all images.", vtrs::RuntimeError::E_TYPE_VK_RESULT, 0);
    }
}

VkShaderModule vtest::VikingRoom::createShaderModule_(const SPIRVCode* const spv_code) {
    VkShaderModule module {};

    VkShaderModuleCreateInfo module_info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spv_code->size,
        .pCode = reinterpret_cast<uint32_t*>(spv_code->data)
    };

    auto result = vkCreateShaderModule(m_device, &module_info, nullptr, &module);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create shader module.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    return module;
}

void vtest::VikingRoom::createRenderPass_() {
    VkAttachmentDescription description {
        .format = m_imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference attachment {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment
    };

    VkRenderPassCreateInfo render_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &description,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    auto result = vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_renderPass);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create render pass.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "DanglingPointer"
void vtest::VikingRoom::createGraphicsPipeline_() {
    auto vert_shader = readSPIRVFile_("shaders/triangle-vert.spv");
    auto frag_shader = readSPIRVFile_("shaders/triangle-frag.spv");

    auto vert_module = createShaderModule_(vert_shader);
    auto frag_module = createShaderModule_(frag_shader);

    VkPipelineShaderStageCreateInfo vert_stage_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_module,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo frag_stage_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_module,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info, frag_stage_info};

    std::vector<VkDynamicState> dynamic_states;
    dynamic_states.reserve(2);
    dynamic_states.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamic_states.emplace_back(VK_DYNAMIC_STATE_SCISSOR);

    VkPipelineDynamicStateCreateInfo dynamic_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };

    VkPipelineVertexInputStateCreateInfo vertex_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo assembly_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_extend2D.width),
        .height = static_cast<float>(m_extend2D.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor {
        .offset = {0, 0},
        .extent = m_extend2D
    };

    VkPipelineViewportStateCreateInfo vstate_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo rasterizer_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState color_blend_attachments {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blending {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachments,
    };

    for (int index = 0; index <= 3; index++) {
        color_blending.blendConstants[index] = 0.0f;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    auto result = vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipelineLayout);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create pipeline layout.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    VkGraphicsPipelineCreateInfo pipeline_info {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_info,
        .pInputAssemblyState = &assembly_info,
        .pViewportState = &vstate_info,
        .pRasterizationState = &rasterizer_info,
        .pMultisampleState = &multisample_info,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_info,
        .layout = m_pipelineLayout,
        .renderPass = m_renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    result = vkCreateGraphicsPipelines(m_device, nullptr, 1, &pipeline_info, nullptr, &m_graphicsPipeline);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create graphics pipeline.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    vkDestroyShaderModule(m_device, vert_module, nullptr);
    vkDestroyShaderModule(m_device, frag_module, nullptr);

    delete vert_shader;
    delete frag_shader;
}
#pragma clang diagnostic pop

void vtest::VikingRoom::createFrameBuffers_() {
    VkResult result = VK_SUCCESS;
    m_swapFramebuffers.resize(m_swapImageViews.size());

    for (size_t index = 0; index < m_swapImageViews.size(); index++) {
        VkImageView attachments[] = {
                m_swapImageViews.at(index)
        };

        VkFramebufferCreateInfo framebuffer_info {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = m_extend2D.width,
            .height = m_extend2D.height,
            .layers = 1
        };

        result = vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &(m_swapFramebuffers.at(index)));

        if(result != VK_SUCCESS) {
            break;
        }
    }

    if (m_swapFramebuffers.size() < m_swapImageViews.size()) {
        throw vtrs::RuntimeError("Unable to create frame buffers.", vtrs::RuntimeError::E_TYPE_GENERAL, result);
    }
}

void vtest::VikingRoom::createCommandPool_(QueueFamilyIndices indices) {
    VkCommandPoolCreateInfo pool_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = indices.graphicsIndex.value()
    };

    auto result = vkCreateCommandPool(m_device, &pool_info, nullptr, &m_commandPool);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create command pool.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }
}

void vtest::VikingRoom::createCommandBuffer_() {
    VkCommandBufferAllocateInfo buffer_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    auto result = vkAllocateCommandBuffers(m_device, &buffer_info, &m_commandBuffer);
    ASSERT_VK_RESULT(result, "Unable to allocate command buffer.")
}

void vtest::VikingRoom::createSyncObjects_() {
    VkSemaphoreCreateInfo semaphoreInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    auto result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSem);
    ASSERT_VK_RESULT(result, "Unable to create image available semaphore.")

    result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_rendererFinishedSem);
    ASSERT_VK_RESULT(result, "Unable to create renderer finished semaphore.")

    result = vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFence);
    ASSERT_VK_RESULT(result, "Unable to create synchronization objects for a frame.")
}

void vtest::VikingRoom::recordCommandBuffer_(VkCommandBuffer command_buffer, uint32_t index) {
    VkCommandBufferBeginInfo begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    auto result = vkBeginCommandBuffer(m_commandBuffer, &begin_info);
    ASSERT_VK_RESULT(result, "Unable to start recording command buffer.")

    VkClearValue clear_color = {{{
        abs(s_bgColor.red),
        abs(s_bgColor.green),
        abs(s_bgColor.blue),
        s_bgColor.alpha
    }}};

    s_bgColor.red += 0.0001f;
    s_bgColor.green += 0.0003f;
    s_bgColor.blue += 0.0006f;

    if (s_bgColor.red >= 1.0f) s_bgColor.red = -0.0001f;
    else if (s_bgColor.red <= 0) s_bgColor.red = 0.0001;

    if (s_bgColor.green >= 1.0f) s_bgColor.green = -0.0003f;
    else if (s_bgColor.green <= 0) s_bgColor.green = 0.0003f;

    if (s_bgColor.blue >= 1.0f) s_bgColor.blue = -0.0006f;
    else if (s_bgColor.blue <= 0) s_bgColor.blue = 0.0006;

    VkRenderPassBeginInfo renderer_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_renderPass,
        .framebuffer = m_swapFramebuffers.at(index),
        .clearValueCount = 1,
        .pClearValues = &clear_color
    };

    renderer_pass_info.renderArea.offset = {0, 0};
    renderer_pass_info.renderArea.extent = m_extend2D;

    vkCmdBeginRenderPass(command_buffer, &renderer_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_extend2D.width),
        .height = static_cast<float>(m_extend2D.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor {
        .offset = {0, 0},
        .extent = m_extend2D
    };

    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    result = vkEndCommandBuffer(command_buffer);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to end recording command buffer.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }
}

void vtest::VikingRoom::bootstrap_() {
    vtest::QueueFamilyIndices indices = findQueueFamilies_();

    if (!indices.graphicsIndex.has_value() || !indices.surfaceIndex.has_value()) {
        abortBootstrap_();
        throw vtrs::RuntimeError("Unable to obtain required queue families.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    initDevice_(indices);
    createSwapchain_(indices);
    createImageViews_();
    createRenderPass_();
    createGraphicsPipeline_();
    createFrameBuffers_();
    createCommandPool_(indices);
    createCommandBuffer_();
    createSyncObjects_();
}

void vtest::VikingRoom::prepareSurface_(vtrs::XCBConnection* connection, uint32_t window) {
    VkXcbSurfaceCreateInfoKHR surface_info {};
    surface_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surface_info.connection = connection;
    surface_info.window = window;

    auto result = vkCreateXcbSurfaceKHR(m_instance, &surface_info, nullptr, &this->m_surface);

    if (result != VK_SUCCESS) {
        vkDestroyInstance(m_instance, nullptr);
        throw vtrs::RuntimeError("Unable to create XCB surface.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }
}

void vtest::VikingRoom::prepareSurface_(vtrs::WLDisplay* display, vtrs::WLSurface* surface) {
    VkWaylandSurfaceCreateInfoKHR surface_info {};
    surface_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    surface_info.display = display;
    surface_info.surface = surface;

    auto result = vkCreateWaylandSurfaceKHR(m_instance, &surface_info, nullptr, &this->m_surface);

    if (result != VK_SUCCESS) {
        vkDestroyInstance(m_instance, nullptr);
        throw vtrs::RuntimeError("Unable to create Wayland surface.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }
}

/**
 * Initialises the instance.
 *
 * Vulkan instance and GPU are initialised here.
 */
vtest::VikingRoom::VikingRoom(std::vector<const char*>& extensions) :
        m_instance{},
        m_device{},
        m_surface{},
        m_queues{},
        m_swapchain{},
        m_swapImages{},
        m_swapImageViews{},
        m_gpu(VK_NULL_HANDLE),
        m_imageFormat(VK_FORMAT_UNDEFINED),
        m_extend2D{},
        m_pipelineLayout{},
        m_renderPass{},
        m_graphicsPipeline{},
        m_commandPool{},
        m_commandBuffer{},
        m_rendererFinishedSem{},
        m_imageAvailableSem{},
        m_inFlightFence{} {
    initVulkan_(extensions);
    initGPU_();
}

/**
 * Public member function definitions.
 *
 * ========================================================================
 */

vtest::VikingRoom* vtest::VikingRoom::factory(vtrs::XCBConnection* connection, uint32_t window) {
    if (!s_isInitialised) {
        enumerateExtensions_();
        s_isInitialised = true;
    }

    std::vector<const char*> extensions({
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    });

    auto instance = new VikingRoom(extensions);
    instance->prepareSurface_(connection, window);
    instance->bootstrap_();

    return instance;
}

vtest::VikingRoom* vtest::VikingRoom::factory(vtrs::WLDisplay* display, vtrs::WLSurface* surface) {
    if (!s_isInitialised) {
        enumerateExtensions_();
        s_isInitialised = true;
    }

    std::vector<const char*> extensions({
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
    });

    auto instance = new VikingRoom(extensions);
    instance->prepareSurface_(display, surface);
    instance->bootstrap_();

    return instance;
}

/**
 * Cleans up.
 */
vtest::VikingRoom::~VikingRoom() {
    vtrs::Logger::info("Cleaning up Vulkan application.");
    vkDeviceWaitIdle(m_device);

    vkDestroySemaphore(m_device, m_imageAvailableSem, nullptr);
    vkDestroySemaphore(m_device, m_rendererFinishedSem, nullptr);
    vkDestroyFence(m_device, m_inFlightFence, nullptr);

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    for (auto buffer : m_swapFramebuffers) {
        vkDestroyFramebuffer(m_device, buffer, nullptr);
    }

    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);

    for (auto image_view : m_swapImageViews) {
        vkDestroyImageView(m_device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    vtrs::Logger::info("Done.");
}

void vtest::VikingRoom::drawFrame() {
    vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFence);

    uint32_t image_index;
    vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSem, VK_NULL_HANDLE, &image_index);

    vkResetCommandBuffer(m_commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer_(m_commandBuffer, image_index);

    VkSemaphore wait_semaphores[] = {m_imageAvailableSem};
    VkSemaphore signal_semaphores[] = {m_rendererFinishedSem};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores
    };

    auto result = vkQueueSubmit(m_queues.graphicsQueue, 1, &submitInfo, m_inFlightFence);
    ASSERT_VK_RESULT(result, "Failed to submit draw command buffer.")

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapChains[] = {m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &image_index;

    vkQueuePresentKHR(m_queues.surfaceQueue, &presentInfo);
}

/**
 * Prints the properties of the selected GPU.
 */
void vtest::VikingRoom::printGPUInfo() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_gpu, &properties);

    const char* type;

    switch (properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            type = "Discrete GPU";
            break;

        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            type = "Integrated GPU";
            break;

        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            type = "CPU";
            break;

        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            type = "Virtual GPU";
            break;

        default:
            type = "Unknown";
    }

    vtrs::Logger::print("Selected GPU Information");
    vtrs::Logger::print("************************");
    vtrs::Logger::print("Device Id:", properties.deviceID);
    vtrs::Logger::print("Vendor Id:", properties.vendorID);
    vtrs::Logger::print("Device Name:", properties.deviceName);
    vtrs::Logger::print("Device Type:", type);
    vtrs::Logger::print("API Version:", properties.apiVersion);
    vtrs::Logger::print("");
}

void vtest::VikingRoom::printIExtensions() {
    vtrs::Logger::print("Available Instance Extensions");
    vtrs::Logger::print("*****************************");

    for (auto& name : s_iExtensions) {
        vtrs::Logger::print(name);
    }

    vtrs::Logger::print("");
}

void vtest::VikingRoom::printGPUExtensions() {
    vtrs::Logger::print("Available GPU Extensions");
    vtrs::Logger::print("************************");

    for (auto& name : m_gExtensions) {
        vtrs::Logger::print(name);
    }

    vtrs::Logger::print("");
}

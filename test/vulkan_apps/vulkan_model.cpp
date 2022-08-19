/**
 * vulkan_model.hpp - Vitreous model test application definition.
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2021-present Ajay Sreedhar
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

#include <vector>
#include <set>
#include <cstring>
#include <limits>
#include <fstream>
#include "platform/logger.hpp"
#include "platform/linux/xcb_client.hpp"
#include "renderer/except.hpp"
#include "renderer/assert.hpp"
#include "vulkan_model.hpp"

vtrs::GPUDevice *vtest::VulkanModel::findDiscreteGPU_() {
    vtrs::GPUDevice* device = vtrs::RendererContext::getGPUList().front();

    for (auto next : vtrs::RendererContext::getGPUList()) {
        if (next->getGPUScore() > device->getGPUScore()) {
            device = next;
        }
    }

    return device;
}

struct vtest::SPIRVBytes vtest::VulkanModel::readSPIRVShader(const std::string &path) {
    std::ifstream shader_file(path, std::ios::ate | std::ios::binary);

    if (!shader_file.is_open()) {
        throw vtrs::RuntimeError("Unable to open shader file " + path, vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    struct vtest::SPIRVBytes spirv_bytes {nullptr,0};

    spirv_bytes.size = shader_file.tellg();
    spirv_bytes.data = new char[spirv_bytes.size]();

    shader_file.seekg(0);
    shader_file.read(spirv_bytes.data, static_cast<std::streamsize>(spirv_bytes.size));
    shader_file.close();

    return spirv_bytes;
}

void vtest::VulkanModel::createSurface_(vtrs::XCBConnection* connection, uint32_t window) {
    VkXcbSurfaceCreateInfoKHR surface_info {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
    surface_info.connection = connection;
    surface_info.window = window;

    auto result = vkCreateXcbSurfaceKHR(vtrs::RendererContext::getInstanceHandle(), &surface_info, nullptr, &m_surface);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create XCB surface.")
}

void vtest::VulkanModel::createLogicalDevice_() {
    float queue_priority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queue_info {};
    std::set<uint32_t> required_indices {m_familyIndices.graphicsFamily.value(), m_familyIndices.surfaceFamily.value()};

    for (auto required_index : required_indices) {
        VkDeviceQueueCreateInfo info {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        info.queueFamilyIndex = required_index;
        info.pQueuePriorities = &queue_priority;
        info.queueCount = 1;

        queue_info.push_back(info);
    }

    VkPhysicalDeviceFeatures gpu_features {};
    std::vector<const char*> req_extensions;
    req_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkDeviceCreateInfo device_info {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.pQueueCreateInfos = queue_info.data();
    device_info.queueCreateInfoCount = queue_info.size();
    device_info.ppEnabledExtensionNames = req_extensions.data();
    device_info.enabledExtensionCount = req_extensions.size();
    device_info.pEnabledFeatures = &gpu_features;

    auto result = vkCreateDevice(m_gpu->getDeviceHandle(), &device_info, nullptr, &m_device);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create logical device.")

    vkGetDeviceQueue(m_device, m_familyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_familyIndices.surfaceFamily.value(), 0, &m_surfaceQueue);
}

void vtest::VulkanModel::createSwapchain_() {
    /* Checking if required extensions for swapchain are supported by the GPU. */
    int extension_flag = 0;
    std::vector<const char*> extension_names = m_gpu->getExtensionNames();

    for(auto& name : extension_names) {
        if (strcmp(name, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            extension_flag++;
            break;
        }
    }

    extension_names.clear();

    if (extension_flag <= 0) {
        throw vtrs::RuntimeError("Selected GPU does not support required swapchain extension.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    /**
     * Query swapchain support details before creating a swapchain.
     * There are basically three kinds of properties we need to check:
     * - Min/max number of images in swap chain,
     *   min/max width and height of images.
     * - Surface formats (pixel format, color space).
     * -Available presentation modes.
     */
    uint32_t format_count; // Surface format count.
    uint32_t mode_count;   // Present mode count.
    struct SwapchainSupportBundle support_bundle;

    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu->getDeviceHandle(), m_surface, &(support_bundle.surfaceCaps));
    VTRS_ASSERT_VK_RESULT(result, "Unable to query surface capabilities of selected GPU.")

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu->getDeviceHandle(), m_surface, &format_count, nullptr);
    VTRS_ASSERT_VK_RESULT(result, "Unable to query supported surface formats provided by the selected GPU.")

    if (format_count <= 0) {
        throw vtrs::RuntimeError("Selected GPU did not provide supported surface format count.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    support_bundle.surfaceFormats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu->getDeviceHandle(), m_surface, &format_count, support_bundle.surfaceFormats.data());

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu->getDeviceHandle(), m_surface, &mode_count, nullptr);
    VTRS_ASSERT_VK_RESULT(result, "Unable to query supported presentation modes provided by the selected GPU.")

    if (mode_count <= 0) {
        throw vtrs::RuntimeError("Selected GPU did not provide supported presentation mode count.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    support_bundle.presentModes.resize(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu->getDeviceHandle(), m_surface, &mode_count, support_bundle.presentModes.data());

    VkSwapchainCreateInfoKHR swapchain_info {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_info.surface = m_surface;
    swapchain_info.minImageCount = support_bundle.surfaceCaps.minImageCount + 1;

    if (support_bundle.surfaceCaps.maxImageCount > 0 && swapchain_info.minImageCount > support_bundle.surfaceCaps.maxImageCount)
        swapchain_info.minImageCount = support_bundle.surfaceCaps.maxImageCount;

    swapchain_info.imageFormat = support_bundle.surfaceFormats.at(0).format;
    swapchain_info.imageColorSpace = support_bundle.surfaceFormats.at(0).colorSpace;

    for (auto& this_format : support_bundle.surfaceFormats) {
        if (this_format.format == VK_FORMAT_B8G8R8A8_SRGB && this_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain_info.imageFormat = this_format.format;
            swapchain_info.imageColorSpace = this_format.colorSpace;
            break;
        }
    }

    if (support_bundle.surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        swapchain_info.imageExtent = support_bundle.surfaceCaps.currentExtent;

    } else {
        VkExtent2D extend_2d;
        extend_2d.width = 800;
        extend_2d.height = 600;

        swapchain_info.imageExtent = extend_2d;
    }

    /* The imageArrayLayers specifies the amount of layers each
     * image consists of. This is always 1 unless we are developing
     * a stereoscopic 3D application.  */
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (m_familyIndices.surfaceFamily == m_familyIndices.graphicsFamily) {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;

    } else {
        uint32_t family_indices[] = {m_familyIndices.surfaceFamily.value(), m_familyIndices.graphicsFamily.value()};

        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = family_indices;
    }

    swapchain_info.preTransform = support_bundle.surfaceCaps.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(m_device, &swapchain_info, nullptr, &m_swapchain);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create swapchain.")

    m_swapExtend = swapchain_info.imageExtent;
    m_swapFormat = swapchain_info.imageFormat;

    uint32_t image_count;
    result = vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
    VTRS_ASSERT_VK_RESULT(result, "Unable to obtain swap chain images.")

    m_swapImages.resize(image_count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_swapImages.data());
}

void vtest::VulkanModel::createImageViews_() {
    m_swapViews.resize(m_swapImages.size());

    for (size_t index = 0; index < m_swapImages.size(); index++) {
        VkImageViewCreateInfo image_view_info {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        image_view_info.image = m_swapImages.at(index);
        image_view_info.format = m_swapFormat;
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.layerCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;

        auto result = vkCreateImageView(m_device, &image_view_info, nullptr, &(m_swapViews.at(index)));
        VTRS_ASSERT_VK_RESULT(result, "Unable to create image view for image.")
    }
}

VkShaderModule vtest::VulkanModel::newShaderModule(struct vtest::SPIRVBytes spirv) {
    VkShaderModuleCreateInfo module_info {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module_info.codeSize = spirv.size;
    module_info.pCode = reinterpret_cast<const uint32_t*>(spirv.data);

    VkShaderModule shader_module;

    auto result = vkCreateShaderModule(m_device, &module_info, nullptr, &shader_module);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create SPIR-V shader module.")

    return shader_module;
}

void vtest::VulkanModel::createRenderPass_() {
    VkAttachmentDescription color_attachment {
        VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        m_swapFormat,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference attachment_ref {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass_desc {};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_desc;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    auto result = vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_renderPass);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create render pass.")
}

void vtest::VulkanModel::setupGraphicsPipeline_() {
    auto vertex_bytes = readSPIRVShader("shaders/triangle-vert.spv");
    auto fragment_bytes = readSPIRVShader("shaders/triangle-frag.spv");

    VkShaderModule vert_shader_module = newShaderModule(vertex_bytes);
    VkShaderModule frag_shader_module = newShaderModule(fragment_bytes);

    VkPipelineShaderStageCreateInfo vert_stage_info {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_shader_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_shader_module;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info, frag_stage_info};

    std::vector<VkDynamicState> dynamic_state_list = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_info.dynamicStateCount = dynamic_state_list.size();
    dynamic_state_info.pDynamicStates = dynamic_state_list.data();

    VkPipelineVertexInputStateCreateInfo vertex_input_info {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo vertex_assembly_info {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    vertex_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    vertex_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport_region {0.0f, 0.0f};
    viewport_region.width = static_cast<float>(m_swapExtend.width);
    viewport_region.height = static_cast<float>(m_swapExtend.height);
    viewport_region.minDepth = 0.0f;
    viewport_region.maxDepth = 1.0f;

    VkRect2D scissor_region { {0, 0}, m_swapExtend };

    VkPipelineViewportStateCreateInfo viewport_info {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_info.viewportCount = 1;
    viewport_info.pViewports = &viewport_region;
    viewport_info.scissorCount = 1;
    viewport_info.pScissors = &scissor_region;

    VkPipelineRasterizationStateCreateInfo rasterizer_info {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_info.lineWidth = 1.0f;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample_info {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample_info.sampleShadingEnable = VK_FALSE;
    multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment {
        VK_TRUE,
        VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blend_info {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;

    for (int index = 0; index <= 3; index++) {
        color_blend_info.blendConstants[index] = 0.0f;
    }

    VkPipelineLayoutCreateInfo layout_info {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = 0; // Optional
    layout_info.pSetLayouts = nullptr; // Optional
    layout_info.pushConstantRangeCount = 0; // Optional
    layout_info.pPushConstantRanges = nullptr; // Optional

    auto result = vkCreatePipelineLayout(m_device, &layout_info, nullptr, &m_pipelineLayout);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create pipeline layout.")

    VkGraphicsPipelineCreateInfo pipeline_info {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState= &vertex_assembly_info;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisample_info;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.layout = m_pipelineLayout;
    pipeline_info.renderPass = m_renderPass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphicsPipeline);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create graphics pipeline.")

    vkDestroyShaderModule(m_device, vert_shader_module, nullptr);
    vkDestroyShaderModule(m_device, frag_shader_module, nullptr);
}

void vtest::VulkanModel::createFramebuffers_() {
    m_swapFramebuffers.resize(m_swapViews.size());

    VkResult result;
    for (size_t index = 0; index < m_swapViews.size(); index++) {
        VkImageView attachments[] = { m_swapViews.at(index) };

        VkFramebufferCreateInfo framebuffer_info {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass = m_renderPass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = m_swapExtend.width;
        framebuffer_info.height = m_swapExtend.height;
        framebuffer_info.layers = 1;

        result = vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &(m_swapFramebuffers.at(index)));
        VTRS_ASSERT_VK_RESULT(result, "Unable to create frame buffer")
    }
}

void vtest::VulkanModel::createCommandPool_() {
    VkCommandPoolCreateInfo pool_info {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = m_familyIndices.graphicsFamily.value();

    auto result = vkCreateCommandPool(m_device, &pool_info, nullptr, &m_commandPool);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create command pool.")
}

void vtest::VulkanModel::allocateCommandBuffers_() {
    VkCommandBufferAllocateInfo buffer_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    buffer_info.commandPool = m_commandPool;
    buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_info.commandBufferCount = m_commandBuffers.size();

    auto result = vkAllocateCommandBuffers(m_device, &buffer_info, m_commandBuffers.data());
    VTRS_ASSERT_VK_RESULT(result, "Unable to allocate command buffers.")
}

void vtest::VulkanModel::recordCommands_(VkCommandBuffer buffer, uint32_t image_index) {
    VkCommandBufferBeginInfo command_buffer_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    auto result = vkBeginCommandBuffer(buffer, &command_buffer_info);
    VTRS_ASSERT_VK_RESULT(result, "Unable to start recording command buffer.")

    VkClearValue clear_colour {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo render_pass_info {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render_pass_info.renderPass = m_renderPass;
    render_pass_info.framebuffer = m_swapFramebuffers.at(image_index);
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = m_swapExtend;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_colour;

    vkCmdBeginRenderPass(buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    VkViewport viewport {0.0f, 0.0f};
    viewport.width = static_cast<float>(m_swapExtend.width);
    viewport.height = static_cast<float>(m_swapExtend.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissor{{ 0, 0 }, m_swapExtend};
    vkCmdSetScissor(buffer, 0, 1, &scissor);

    vkCmdDraw(buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(buffer);

    result = vkEndCommandBuffer(buffer);
    VTRS_ASSERT_VK_RESULT(result, "Unable to stop recording command buffer.")
}

void vtest::VulkanModel::createSyncObjects_() {
    VkSemaphoreCreateInfo semaphore_info {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t index = 0; index < VTEST_MAX_FRAMES_IN_FLIGHT; index++) {
        auto result = vkCreateSemaphore(m_device, &semaphore_info, nullptr, &(m_syncObjects.imageAvailableSem.at(index)));
        VTRS_ASSERT_VK_RESULT(result, "Unable to obtain image synchronization semaphore.")

        result = vkCreateSemaphore(m_device, &semaphore_info, nullptr, &(m_syncObjects.renderFinishedSem.at(index)));
        VTRS_ASSERT_VK_RESULT(result, "Unable to obtain renderer synchronization semaphore.")

        result = vkCreateFence(m_device, &fence_info, nullptr, &(m_syncObjects.inFlightFence.at(index)));
        VTRS_ASSERT_VK_RESULT(result, "Unable to obtain in-flight fence.")
    }
}

void vtest::VulkanModel::bootstrap_() {
    m_familyIndices.graphicsFamily = m_gpu->getQueueFamilyIndex(vtrs::GPUDevice::QUEUE_FAMILY_INDEX_GRAPHICS);

    for (uint32_t index = 0; index < m_gpu->getQueueFamilyCount(); index++) {
        VkBool32 is_supported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu->getDeviceHandle(), index, m_surface, &is_supported);

        if (is_supported) {
            m_familyIndices.surfaceFamily = index;
            break;
        }
    }

    createLogicalDevice_();
    createSwapchain_();
    createImageViews_();
    createRenderPass_();
    setupGraphicsPipeline_();
    createFramebuffers_();
    createCommandPool_();
    allocateCommandBuffers_();
    createSyncObjects_();
}

vtest::VulkanModel *vtest::VulkanModel::factory(vtrs::XCBClient* client, vtrs::XCBWindow window) {
    auto application = new VulkanModel();
    application->createSurface_(client->getConnection(), window.identifier);
    application->bootstrap_();

    return application;
}


vtest::VulkanModel::VulkanModel() {
    try {
        vtrs::RendererContext::initialise();

    } catch (vtrs::RendererError&) {}

    m_gpu = findDiscreteGPU_();

    m_commandBuffers.resize(VTEST_MAX_FRAMES_IN_FLIGHT);

    m_syncObjects.imageAvailableSem.resize(VTEST_MAX_FRAMES_IN_FLIGHT);
    m_syncObjects.renderFinishedSem.resize(VTEST_MAX_FRAMES_IN_FLIGHT);
    m_syncObjects.inFlightFence.resize(VTEST_MAX_FRAMES_IN_FLIGHT);
}

vtest::VulkanModel::~VulkanModel() {
    vtrs::Logger::info("Cleaning up Vulkan Model application.");

    for (size_t index = 0; index < VTEST_MAX_FRAMES_IN_FLIGHT; index++) {
        vkDestroySemaphore(m_device, m_syncObjects.imageAvailableSem.at(index), nullptr);
        vkDestroySemaphore(m_device, m_syncObjects.renderFinishedSem.at(index), nullptr);
        vkDestroyFence(m_device, m_syncObjects.inFlightFence.at(index), nullptr);
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    for (auto framebuffer : m_swapFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for(auto image_view : m_swapViews) {
        vkDestroyImageView(m_device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(vtrs::RendererContext::getInstanceHandle(), m_surface, nullptr);

    m_syncObjects.imageAvailableSem.clear();
    m_syncObjects.renderFinishedSem.clear();
    m_syncObjects.inFlightFence.clear();

    m_swapViews.clear();
    m_swapImages.clear();
    m_swapFramebuffers.clear();
    m_commandBuffers.clear();

    m_gpu = nullptr;
}

bool vtest::VulkanModel::drawFrame() {
    vkWaitForFences(m_device, 1, &(m_syncObjects.inFlightFence.at(m_currentFrame)), VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    auto result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_syncObjects.imageAvailableSem.at(m_currentFrame), VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        rebuildSwapchain();
        return false;
    }

    VTRS_ASSERT_VK_RESULT(result, "Unable to obtain next image from swapchain")

    vkResetFences(m_device, 1, &(m_syncObjects.inFlightFence.at(m_currentFrame)));

    vkResetCommandBuffer(m_commandBuffers.at(m_currentFrame), /*VkCommandBufferResetFlagBits*/ 0);
    recordCommands_(m_commandBuffers.at(m_currentFrame), image_index);

    VkSubmitInfo submit_info {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    VkSemaphore wait_semaphores[] = {m_syncObjects.imageAvailableSem.at(m_currentFrame)};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_commandBuffers.at(m_currentFrame);

    VkSemaphore signal_semaphores[] = {m_syncObjects.renderFinishedSem.at(m_currentFrame)};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    result = vkQueueSubmit(m_graphicsQueue, 1, &submit_info, m_syncObjects.inFlightFence.at(m_currentFrame));
    VTRS_ASSERT_VK_RESULT(result, "Failed to submit command buffer to queue.")

    VkPresentInfoKHR present_info {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {m_swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(m_surfaceQueue, &present_info);

    m_currentFrame = (m_currentFrame + 1) % VTEST_MAX_FRAMES_IN_FLIGHT;
    return true;
}

void vtest::VulkanModel::printGPUInfo() {
    m_gpu->printInfo();
}

void vtest::VulkanModel::waitIdle() {
    vkDeviceWaitIdle(m_device);
}

void vtest::VulkanModel::rebuildSwapchain() {
    if (m_swapchain == VK_NULL_HANDLE) {
        throw vtrs::RuntimeError("Swapchain should be built first.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    waitIdle();

    for (auto buffer : m_swapFramebuffers) {
        vkDestroyFramebuffer(m_device, buffer, nullptr);
    }

    for(auto image_view : m_swapViews) {
        vkDestroyImageView(m_device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    createSwapchain_();
    createImageViews_();
    createFramebuffers_();
}
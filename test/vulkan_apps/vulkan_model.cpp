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

#define STB_IMAGE_IMPLEMENTATION 1
#define TINYOBJLOADER_IMPLEMENTATION 1

#include <set>
#include <unordered_map>
#include <cstring>
#include <limits>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include "third_party/stb/stb_image.h"
#include "third_party/tiny_object_loader/tiny_obj_loader.h"
#include "platform/logger.hpp"
#include "platform/linux/xcb_client.hpp"
#include "renderer/except.hpp"
#include "renderer/assert.hpp"
#include "vulkan_model.hpp"

std::vector<vtest::Vertex> vtest::VulkanModel::s_vertices {};
std::vector<uint32_t> vtest::VulkanModel::s_indices {};

VkVertexInputBindingDescription vtest::Vertex::getInputBindingDescription() {
    VkVertexInputBindingDescription description {0, sizeof(vtest::Vertex), VK_VERTEX_INPUT_RATE_VERTEX};

    return description;
}

std::array<VkVertexInputAttributeDescription, 3> vtest::Vertex::getInputAttributeDescription() {
    std::array<VkVertexInputAttributeDescription, 3> descriptions {};

    descriptions.at(0).binding = 0;
    descriptions.at(0).location = 0;
    descriptions.at(0).format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions.at(0).offset = offsetof(vtest::Vertex, coordinate);

    descriptions.at(1).binding = 0;
    descriptions.at(1).location = 1;
    descriptions.at(1).format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions.at(1).offset = offsetof(vtest::Vertex, rgbColor);

    descriptions.at(2).binding = 0;
    descriptions.at(2).location = 2;
    descriptions.at(2).format = VK_FORMAT_R32G32_SFLOAT;
    descriptions.at(2).offset = offsetof(vtest::Vertex, textureXY);

    return descriptions;
}

vtrs::RendererGPU *vtest::VulkanModel::findDiscreteGPU_() {
    vtrs::RendererGPU* device = vtrs::RendererContext::getGPUList().front();

    for (auto next : vtrs::RendererContext::getGPUList()) {
        if (next->getGPUScore() > device->getGPUScore()) {
            device = next;
        }
    }

    return device;
}

bool vtest::VulkanModel::hasStencilComponent_(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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

uint32_t vtest::VulkanModel::findMemoryType_(uint32_t filter, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties mem_props {};
    vkGetPhysicalDeviceMemoryProperties(m_gpu->getDeviceHandle(), &mem_props);

    for (unsigned int index = 0; index < mem_props.memoryTypeCount; index++) {
        if ((filter & (1 << index)) && (mem_props.memoryTypes[index].propertyFlags & flags) == flags) {
            return index;
        }
    }

    throw vtrs::RuntimeError("Unable to find required memory type.", vtrs::RuntimeError::E_TYPE_GENERAL);
}

VkFormat vtest::VulkanModel::findSupportedFormat_(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties properties {};
        vkGetPhysicalDeviceFormatProperties(m_gpu->getDeviceHandle(), format, &properties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            return format;
        }

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw vtrs::RuntimeError("Failed to find supported format!", vtrs::RuntimeError::E_TYPE_GENERAL);
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
    std::set<uint32_t> required_indices {
        m_familyIndices.graphicsFamily.value(),
        m_familyIndices.transferFamily.value(),
        m_familyIndices.surfaceFamily.value(),
    };

    for (auto required_index : required_indices) {
        VkDeviceQueueCreateInfo info {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        info.queueFamilyIndex = required_index;
        info.pQueuePriorities = &queue_priority;
        info.queueCount = 1;

        queue_info.push_back(info);
    }

    VkPhysicalDeviceFeatures gpu_features {};
    gpu_features.samplerAnisotropy = VK_TRUE;

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
    vkGetDeviceQueue(m_device, m_familyIndices.transferFamily.value(), 0, &m_transferQueue);
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

        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
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

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = findSupportedFormat_({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                                   VK_IMAGE_TILING_OPTIMAL,
                                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_reference {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_reference {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass_desc {};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_reference;
    subpass_desc.pDepthStencilAttachment = &depth_reference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachment_list {color_attachment, depth_attachment};

    VkRenderPassCreateInfo render_pass_info {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_info.attachmentCount = attachment_list.size();
    render_pass_info.pAttachments = attachment_list.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_desc;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    auto result = vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_renderPass);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create render pass.")
}

void vtest::VulkanModel::createDescSetLayout_() {
    VkDescriptorSetLayoutBinding ubo_binding {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT};
    VkDescriptorSetLayoutBinding sampler_binding {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT};

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {ubo_binding, sampler_binding};

    VkDescriptorSetLayoutCreateInfo layout_info {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = bindings.size();
    layout_info.pBindings = bindings.data();

    auto result = vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_descSetLayout);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create UBO descriptor set layout.")
}

void vtest::VulkanModel::createDescPool_() {
    std::array<VkDescriptorPoolSize, 2> pool_sizes {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = static_cast<uint32_t>(VTEST_MAX_FRAMES_IN_FLIGHT);
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = static_cast<uint32_t>(VTEST_MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo pool_info {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.poolSizeCount = pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = VTEST_MAX_FRAMES_IN_FLIGHT;

    auto result = vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_descPool);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create UBO descriptor pool.")
}

void vtest::VulkanModel::createDescSets_() {
    std::vector<VkDescriptorSetLayout> layouts (VTEST_MAX_FRAMES_IN_FLIGHT, m_descSetLayout);

    VkDescriptorSetAllocateInfo alloc_info {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = m_descPool;
    alloc_info.descriptorSetCount = VTEST_MAX_FRAMES_IN_FLIGHT;
    alloc_info.pSetLayouts = layouts.data();

    m_descSets.resize(VTEST_MAX_FRAMES_IN_FLIGHT);

    auto result = vkAllocateDescriptorSets(m_device, &alloc_info, m_descSets.data());
    VTRS_ASSERT_VK_RESULT(result, "Unable to create descriptor sets.")

    for (size_t index = 0; index < VTEST_MAX_FRAMES_IN_FLIGHT; index++) {
        VkDescriptorBufferInfo buffer_info {
            m_uniformBuffer.at(index),
            0,
            sizeof(vtest::UniformBufferObject)
        };

        VkDescriptorImageInfo image_info {m_textureBundle.sampler, m_textureBundle.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        std::array<VkWriteDescriptorSet, 2> descriptor_writes {};
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = m_descSets.at(index);
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &buffer_info;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = m_descSets.at(index);
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pImageInfo = &image_info;

        vkUpdateDescriptorSets(m_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }
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

    auto vertex_bind_desc = vtest::Vertex::getInputBindingDescription();
    auto vertex_attr_desc = vtest::Vertex::getInputAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertex_input_info {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_bind_desc;
    vertex_input_info.vertexAttributeDescriptionCount = vertex_attr_desc.size();
    vertex_input_info.pVertexAttributeDescriptions = vertex_attr_desc.data();

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
    rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &m_descSetLayout;
    layout_info.pushConstantRangeCount = 0; // Optional
    layout_info.pPushConstantRanges = nullptr; // Optional

    auto result = vkCreatePipelineLayout(m_device, &layout_info, nullptr, &m_pipelineLayout);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create pipeline layout.")

    VkPipelineDepthStencilStateCreateInfo depth_stencil_info {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_stencil_info.depthTestEnable = VK_TRUE;
    depth_stencil_info.depthWriteEnable = VK_TRUE;
    depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_info.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipeline_info {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState= &vertex_assembly_info;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisample_info;
    pipeline_info.pDepthStencilState = &depth_stencil_info;
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

void vtest::VulkanModel::createDepthResources_() {
    VkFormat depth_format = findSupportedFormat_({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                                VK_IMAGE_TILING_OPTIMAL,
                                                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    struct ImageObjectBundle bundle = createImage_(m_swapExtend.width, m_swapExtend.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_depthResource.image = bundle.image;
    m_depthResource.view = bundle.view;
    m_depthResource.memory = bundle.memory;

    VkImageViewCreateInfo view_info {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = m_depthResource.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = depth_format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    auto result = vkCreateImageView(m_device, &view_info, nullptr, &(m_depthResource.view));
    VTRS_ASSERT_VK_RESULT(result, "Method createDepthResources_ failed created while creating image views.")
}

void vtest::VulkanModel::createFramebuffers_() {
    m_swapFramebuffers.resize(m_swapViews.size());

    VkResult result;
    for (size_t index = 0; index < m_swapViews.size(); index++) {
        VkImageView attachments[] = { m_swapViews.at(index), m_depthResource.view };

        VkFramebufferCreateInfo framebuffer_info {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass = m_renderPass;
        framebuffer_info.attachmentCount = 2;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = m_swapExtend.width;
        framebuffer_info.height = m_swapExtend.height;
        framebuffer_info.layers = 1;

        result = vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &(m_swapFramebuffers.at(index)));
        VTRS_ASSERT_VK_RESULT(result, "Unable to create frame buffer")
    }
}

void vtest::VulkanModel::createCommandPools_() {
    VkCommandPoolCreateInfo pool_info {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = m_familyIndices.graphicsFamily.value();

    auto result = vkCreateCommandPool(m_device, &pool_info, nullptr, &m_graphicsCmdPool);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create Graphics Command Pool.")

    pool_info.queueFamilyIndex = m_familyIndices.transferFamily.value();
    result = vkCreateCommandPool(m_device, &pool_info, nullptr, &m_transferCmdPool);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create Transfer Command Pool.")
}

void vtest::VulkanModel::allocateCommandBuffers_() {
    VkCommandBufferAllocateInfo buffer_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    buffer_info.commandPool = m_graphicsCmdPool;
    buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_info.commandBufferCount = m_commandBuffers.size();

    auto result = vkAllocateCommandBuffers(m_device, &buffer_info, m_commandBuffers.data());
    VTRS_ASSERT_VK_RESULT(result, "Unable to allocate command buffers.")
}

void vtest::VulkanModel::recordCommands_(VkCommandBuffer command_buffer, uint32_t image_index) {
    VkCommandBufferBeginInfo command_buffer_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    auto result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
    VTRS_ASSERT_VK_RESULT(result, "Unable to start recording command command_buffer.")

    std::array<VkClearValue, 2> clear_colours {};
    clear_colours[0].color = {{0.004f, 0.00266f, 0.0088f, 1.0f}};
    clear_colours[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo render_pass_info {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render_pass_info.renderPass = m_renderPass;
    render_pass_info.framebuffer = m_swapFramebuffers.at(image_index);
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = m_swapExtend;
    render_pass_info.clearValueCount = clear_colours.size();
    render_pass_info.pClearValues = clear_colours.data();

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    VkViewport viewport {0.0f, 0.0f};
    viewport.width = static_cast<float>(m_swapExtend.width);
    viewport.height = static_cast<float>(m_swapExtend.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{{ 0, 0 }, m_swapExtend};
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer vertex_buffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &(m_descSets.at(m_currentFrame)), 0, nullptr);

    vkCmdDrawIndexed(command_buffer, s_indices.size(), 1, 0, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    result = vkEndCommandBuffer(command_buffer);
    VTRS_ASSERT_VK_RESULT(result, "Unable to stop recording command command_buffer.")
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

void vtest::VulkanModel::copyBuffer_(VkBuffer dest_buffer, VkBuffer src_buffer, VkDeviceSize buffer_size) {
    VkCommandBufferAllocateInfo alloc_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = m_transferCmdPool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buffer = VK_NULL_HANDLE;
    auto result = vkAllocateCommandBuffers(m_device, &alloc_info, &cmd_buffer);
    VTRS_ASSERT_VK_RESULT(result, "Private member copyBuffer_ failed while allocating command buffer.")

    VkCommandBufferBeginInfo begin_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(cmd_buffer, &begin_info);
    VTRS_ASSERT_VK_RESULT(result, "Private member copyBuffer_ failed while attempting to record commands.")

    VkBufferCopy copy_region {0, 0, buffer_size};
    vkCmdCopyBuffer(cmd_buffer, src_buffer, dest_buffer, 1, &copy_region);

    result = vkEndCommandBuffer(cmd_buffer);
    VTRS_ASSERT_VK_RESULT(result, "Private member copyBuffer_ failed after copying buffer region.")

    VkSubmitInfo submit_info {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;

    result = vkQueueSubmit(m_transferQueue, 1, &submit_info, VK_NULL_HANDLE);
    VTRS_ASSERT_VK_RESULT(result, "Private member copyBuffer_ failed while submitting command buffer.")

    vkQueueWaitIdle(m_transferQueue);
    vkFreeCommandBuffers(m_device, m_transferCmdPool, 1, &cmd_buffer);
}


vtest::BufferObjectBundle vtest::VulkanModel::createBuffer_(VkDeviceSize buffer_size, VkBufferUsageFlags buffer_flags, VkMemoryPropertyFlags mem_flags) {
    BufferObjectBundle bundle {VK_NULL_HANDLE, VK_NULL_HANDLE};

    VkBufferCreateInfo buffer_info {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = buffer_size;
    buffer_info.usage = buffer_flags;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto result = vkCreateBuffer(m_device, &buffer_info, nullptr, &(bundle.buffer));
    VTRS_ASSERT_VK_RESULT(result, "Unable to create specified buffer buffer.")

    VkMemoryRequirements mem_reqs {};
    vkGetBufferMemoryRequirements(m_device, bundle.buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = findMemoryType_(mem_reqs.memoryTypeBits, mem_flags);

    result = vkAllocateMemory(m_device, &alloc_info, nullptr, &(bundle.memory));
    VTRS_ASSERT_VK_RESULT(result, "Unable to allocate memory for buffer.")

    result = vkBindBufferMemory(m_device, bundle.buffer, bundle.memory, 0);
    VTRS_ASSERT_VK_RESULT(result, "Unable to bind new memory for buffer.")

    return bundle;
}

vtest::ImageObjectBundle
vtest::VulkanModel::createImage_(uint32_t width,
                                 uint32_t height,
                                 VkFormat format,
                                 VkImageTiling tiling,
                                 VkImageUsageFlags usage_flags,
                                 VkMemoryPropertyFlags memory_flags) {

    ImageObjectBundle bundle {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkMemoryRequirements mem_reqs {};

    VkImageCreateInfo image_info {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage_flags;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto result = vkCreateImage(m_device, &image_info, nullptr, &(bundle.image));
    VTRS_ASSERT_VK_RESULT(result, "Failed to create image bundle.")

    vkGetImageMemoryRequirements(m_device, bundle.image, &mem_reqs);

    VkMemoryAllocateInfo alloc_info {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = findMemoryType_(mem_reqs.memoryTypeBits, memory_flags);

    result = vkAllocateMemory(m_device, &alloc_info, nullptr, &(bundle.memory));
    VTRS_ASSERT_VK_RESULT(result, "Failed to allocate memory for image bundle.")

    vkBindImageMemory(m_device, bundle.image, bundle.memory, 0);
    return bundle;
}

void vtest::VulkanModel::createVertexBuffer_() {
    VkDeviceSize buffer_size = sizeof(s_vertices.at(0)) * s_vertices.size();
    BufferObjectBundle staging_bundle = createBuffer_(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    auto result = vkMapMemory(m_device, staging_bundle.memory, 0, buffer_size, 0, &m_vertexData);
    VTRS_ASSERT_VK_RESULT(result, "Unable to map staging vertex memory.")

    memcpy(m_vertexData, s_vertices.data(), static_cast<size_t>(buffer_size));

    BufferObjectBundle local_bundle = createBuffer_(buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_vertexBuffer = local_bundle.buffer;
    m_vertexMemory = local_bundle.memory;

    copyBuffer_(m_vertexBuffer, staging_bundle.buffer, buffer_size);

    vkUnmapMemory(m_device, staging_bundle.memory);

    vkDestroyBuffer(m_device, staging_bundle.buffer, nullptr);
    vkFreeMemory(m_device, staging_bundle.memory, nullptr);
}

void vtest::VulkanModel::createIndexBuffer_() {
    VkDeviceSize buffer_size = sizeof(s_indices.at(0)) * s_indices.size();
    BufferObjectBundle staging_bundle = createBuffer_(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    auto result = vkMapMemory(m_device, staging_bundle.memory, 0, buffer_size, 0, &m_indexData);
    VTRS_ASSERT_VK_RESULT(result, "Unable to map staging index memory.")

    memcpy(m_indexData, s_indices.data(), static_cast<size_t>(buffer_size));

    BufferObjectBundle local_bundle = createBuffer_(buffer_size,
                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_indexBuffer = local_bundle.buffer;
    m_indexMemory = local_bundle.memory;

    copyBuffer_(m_indexBuffer, staging_bundle.buffer, buffer_size);

    vkUnmapMemory(m_device, staging_bundle.memory);
    vkDestroyBuffer(m_device, staging_bundle.buffer, nullptr);
    vkFreeMemory(m_device, staging_bundle.memory, nullptr);
}

void vtest::VulkanModel::createUniformBuffers_() {
    VkDeviceSize buffer_size = sizeof (struct vtest::UniformBufferObject);
    m_uniformBuffer.resize(VTEST_MAX_FRAMES_IN_FLIGHT);
    m_uniformMemory.resize(VTEST_MAX_FRAMES_IN_FLIGHT);

    for (int index = 0; index < VTEST_MAX_FRAMES_IN_FLIGHT; index++) {
        auto bundle = createBuffer_(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_uniformBuffer.at(index) = bundle.buffer;
        m_uniformMemory.at(index) = bundle.memory;
    }
}

void vtest::VulkanModel::copyBufferToImage_(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBufferAllocateInfo alloc_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = m_graphicsCmdPool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    auto result = vkAllocateCommandBuffers(m_device, &alloc_info, &command_buffer);
    VTRS_ASSERT_VK_RESULT(result, "Method copyBufferToImage_ failed while allocating command buffer.")

    VkCommandBufferBeginInfo begin_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_graphicsCmdPool, 1, &command_buffer);
}

void vtest::VulkanModel::transitionImageLayout_(VkImage image, VkFormat, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkCommandBufferAllocateInfo alloc_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = m_graphicsCmdPool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    auto result = vkAllocateCommandBuffers(m_device, &alloc_info, &command_buffer);
    VTRS_ASSERT_VK_RESULT(result, "Method transitionImageLayout_ failed while allocating command buffer.")

    VkCommandBufferBeginInfo begin_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkImageMemoryBarrier barrier {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw vtrs::RuntimeError("Unsupported layout transition.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_graphicsCmdPool, 1, &command_buffer);
}

void vtest::VulkanModel::createTextureImage_(const std::string& file_path) {
    int image_width;
    int image_height;
    int image_channels;

    stbi_uc* pixels = stbi_load(file_path.c_str(), &image_width, &image_height, &image_channels, STBI_rgb_alpha);

    if (pixels == nullptr) {
        throw vtrs::RuntimeError("Unable to load texture image.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    VkDeviceSize image_size = image_width * image_height * 4;
    vtest::BufferObjectBundle staging_bundle = createBuffer_(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* buffer_data;
    vkMapMemory(m_device, staging_bundle.memory, 0, image_size, 0, &buffer_data);
    memcpy(buffer_data, pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(m_device, staging_bundle.memory);

    stbi_image_free(pixels);

    auto image_bundle = createImage_(image_width, image_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_textureBundle.image  = image_bundle.image;
    m_textureBundle.memory = image_bundle.memory;

    transitionImageLayout_(m_textureBundle.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage_(staging_bundle.buffer, m_textureBundle.image, static_cast<uint32_t>(image_width), static_cast<uint32_t>(image_height));
    transitionImageLayout_(m_textureBundle.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageViewCreateInfo view_info {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = m_textureBundle.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    auto result = vkCreateImageView(m_device, &view_info, nullptr, &(m_textureBundle.view));
    VTRS_ASSERT_VK_RESULT(result, "Method createTextureImage_ failed while creating image view.")

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy =  m_gpu->getGPULimit<float>("maxSamplerAnisotropy");
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    result = vkCreateSampler(m_device, &samplerInfo, nullptr, &(m_textureBundle.sampler));
    VTRS_ASSERT_VK_RESULT(result, "Method createTextureImage_ failed while creating sampler")

    vkDestroyBuffer(m_device, staging_bundle.buffer, nullptr);
    vkFreeMemory(m_device, staging_bundle.memory, nullptr);
}

void vtest::VulkanModel::bootstrap_() {
    m_familyIndices.graphicsFamily = m_gpu->getQueueFamilyIndex(vtrs::RendererGPU::QUEUE_FAMILY_INDEX_GRAPHICS);
    m_familyIndices.transferFamily = m_gpu->getQueueFamilyIndex(vtrs::RendererGPU::QUEUE_FAMILY_INDEX_TRANSFER);

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
    createDescSetLayout_();
    setupGraphicsPipeline_();
    createDepthResources_();
    createFramebuffers_();
    createCommandPools_();
    allocateCommandBuffers_();
    createSyncObjects_();
}

void vtest::VulkanModel::updateUniformBuffers_(uint32_t current_frame) const {
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    UniformBufferObject ubo {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    float aspect = static_cast<float>(m_swapExtend.width) / static_cast<float>(m_swapExtend.height);

    ubo.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);

    /* Inverting Y-axis. */
    ubo.projection[1][1] *= -1;

    void* buffer_data;
    vkMapMemory(m_device, m_uniformMemory.at(current_frame), 0, sizeof(ubo), 0, &buffer_data);
    memcpy(buffer_data, &ubo, sizeof(ubo));
    vkUnmapMemory(m_device, m_uniformMemory.at(current_frame));
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

    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexMemory, nullptr);

    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_device, m_indexMemory, nullptr);

    for (size_t index = 0; index < VTEST_MAX_FRAMES_IN_FLIGHT; index++) {
        vkDestroySemaphore(m_device, m_syncObjects.imageAvailableSem.at(index), nullptr);
        vkDestroySemaphore(m_device, m_syncObjects.renderFinishedSem.at(index), nullptr);
        vkDestroyFence(m_device, m_syncObjects.inFlightFence.at(index), nullptr);

        vkDestroyBuffer(m_device, m_uniformBuffer.at(index), nullptr);
        vkFreeMemory(m_device, m_uniformMemory.at(index), nullptr);
    }

    vkDestroyDescriptorPool(m_device, m_descPool, nullptr);
    vkDestroyCommandPool(m_device, m_transferCmdPool, nullptr);
    vkDestroyCommandPool(m_device, m_graphicsCmdPool, nullptr);

    for (auto framebuffer : m_swapFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    vkDestroyDescriptorSetLayout(m_device, m_descSetLayout, nullptr);

    for(auto image_view : m_swapViews) {
        vkDestroyImageView(m_device, image_view, nullptr);
    }

    vkDestroyImageView(m_device, m_depthResource.view, nullptr);
    vkDestroyImage(m_device, m_depthResource.image, nullptr);
    vkFreeMemory(m_device, m_depthResource.memory, nullptr);

    vkDestroyImageView(m_device, m_textureBundle.view, nullptr);
    vkDestroySampler(m_device, m_textureBundle.sampler, nullptr);
    vkDestroyImage(m_device, m_textureBundle.image, nullptr);
    vkFreeMemory(m_device, m_textureBundle.memory, nullptr);

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

    VTRS_ASSERT_VK_RESULT(result, "Unable to obtain next image from swapchain.")

    vkResetFences(m_device, 1, &(m_syncObjects.inFlightFence.at(m_currentFrame)));

    vkResetCommandBuffer(m_commandBuffers.at(m_currentFrame), /*VkCommandBufferResetFlagBits*/ 0);
    recordCommands_(m_commandBuffers.at(m_currentFrame), image_index);

    updateUniformBuffers_(m_currentFrame);

    VkSubmitInfo submit_info {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    VkSemaphore wait_semaphores[] = {m_syncObjects.imageAvailableSem.at(m_currentFrame)};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(m_commandBuffers.at(m_currentFrame));

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

    for (auto buffer: m_swapFramebuffers) {
        vkDestroyFramebuffer(m_device, buffer, nullptr);
    }

    for (auto image_view: m_swapViews) {
        vkDestroyImageView(m_device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    createSwapchain_();
    createImageViews_();

    vkDestroyImageView(m_device, m_depthResource.view, nullptr);
    vkDestroyImage(m_device, m_depthResource.image, nullptr);
    vkFreeMemory(m_device, m_depthResource.memory, nullptr);

    createDepthResources_();
    createFramebuffers_();
}

void vtest::VulkanModel::loadCube(const std::string& texture_file) {
    if (s_vertices.empty()) {
        s_vertices.push_back({{-0.9f, -0.9f, 0.4f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});  // 0
        s_vertices.push_back({{0.9f, -0.9f, 0.4f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});   // 1
        s_vertices.push_back({{0.9f, 0.9f, 0.4f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});    // 2
        s_vertices.push_back({{-0.9f, 0.9f, 0.4f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});   // 3

        s_vertices.push_back({{-0.9f, -0.9f, -0.1f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}); // 4
        s_vertices.push_back({{0.9f, -0.9f, -0.1f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});  // 5
        s_vertices.push_back({{0.9f, 0.9f, -0.1f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});   // 6
        s_vertices.push_back({{-0.9f, 0.9f, -0.1f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});  // 7

        s_indices.push_back(0);
        s_indices.push_back(1);
        s_indices.push_back(2);
        s_indices.push_back(0);
        s_indices.push_back(2);
        s_indices.push_back(3);

        s_indices.push_back(4);
        s_indices.push_back(5);
        s_indices.push_back(6);
        s_indices.push_back(6);
        s_indices.push_back(7);
        s_indices.push_back(4);

        s_indices.push_back(4);
        s_indices.push_back(0);
        s_indices.push_back(7);

        s_indices.push_back(3);
        s_indices.push_back(4);
        s_indices.push_back(7);
    }

    createTextureImage_(texture_file);
    createUniformBuffers_();
    createDescPool_();
    createDescSets_();
    createVertexBuffer_();
    createIndexBuffer_();
}

void vtest::VulkanModel::loadModel(const std::string& texture_file, const std::string& model_file) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_file.c_str())) {
        throw vtrs::RuntimeError(warn + err, vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    std::unordered_map<vtest::Vertex, uint32_t> unique_vertices {};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            vtest::Vertex vertex {};

            vertex.coordinate = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.textureXY = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.rgbColor = {1.0f, 1.0f, 1.0f};

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32_t>(s_vertices.size());
                s_vertices.push_back(vertex);
            }

            s_indices.push_back(unique_vertices[vertex]);
        }
    }

    createTextureImage_(texture_file);
    createUniformBuffers_();
    createDescPool_();
    createDescSets_();
    createVertexBuffer_();
    createIndexBuffer_();
}

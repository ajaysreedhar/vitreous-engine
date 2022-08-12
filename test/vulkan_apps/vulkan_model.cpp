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

    vtrs::Logger::debug("Retrieved", m_swapImages.size(), "swapchain images.");
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

    vkDestroyShaderModule(m_device, vert_shader_module, nullptr);
    vkDestroyShaderModule(m_device, frag_shader_module, nullptr);

    // Remove this if validation layer complaints.
    //free(vertex_bytes.data);
    //free(fragment_bytes.data);
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
    setupGraphicsPipeline_();
}

vtest::VulkanModel *vtest::VulkanModel::factory(vtrs::XCBClient* client, uint32_t window) {
    auto application = new VulkanModel();
    application->createSurface_(client->getConnection(), window);
    application->bootstrap_();

    return application;
}


vtest::VulkanModel::VulkanModel() {
    try {
        vtrs::RendererContext::initialise();

    } catch (vtrs::RendererError&) {}

    m_gpu = findDiscreteGPU_();
}

vtest::VulkanModel::~VulkanModel() {
    vtrs::Logger::info("Cleaning up Vulkan Model application.");

    for(auto image_view : m_swapViews) {
        vkDestroyImageView(m_device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(vtrs::RendererContext::getInstanceHandle(), m_surface, nullptr);

    m_gpu = nullptr;
}

void vtest::VulkanModel::printGPUInfo() {
    m_gpu->printInfo();
}

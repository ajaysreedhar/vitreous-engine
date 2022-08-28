/**
 * surface_presenter.cpp - Infrastructure for presenting on window surface.
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

#include "assert.hpp"
#include "surface_presenter.hpp"

struct vtrs::swapchain_support_bundle
vtrs::SurfacePresenter::querySwapchainSupport_(VkSurfaceKHR surface) {
    uint32_t format_count; // Surface format count.
    uint32_t mode_count;   // Present mode count.
    struct vtrs::swapchain_support_bundle support_bundle {};

    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface, &(support_bundle.surfaceCaps));
    VTRS_ASSERT_VK_RESULT(result, "Unable to query surface capabilities of selected GPU.")

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &format_count, nullptr);
    VTRS_ASSERT_VK_RESULT(result, "Unable to query supported surface formats provided by the selected GPU.")

    if (format_count <= 0) {
        throw vtrs::RendererError("Selected GPU did not provide supported surface format count.", vtrs::RendererError::E_TYPE_GENERAL);
    }

    support_bundle.surfaceFormats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &format_count, support_bundle.surfaceFormats.data());

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &mode_count, nullptr);
    VTRS_ASSERT_VK_RESULT(result, "Unable to query supported presentation modes provided by the selected GPU.")

    if (mode_count <= 0) {
        throw vtrs::RendererError("Selected GPU did not provide supported presentation mode count.", vtrs::RendererError::E_TYPE_GENERAL);
    }

    support_bundle.presentModes.resize(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &mode_count, support_bundle.presentModes.data());

    return support_bundle;
}

void vtrs::SurfacePresenter::createSwapchain_(VkSurfaceKHR surface, struct vtrs::surface_presenter_opts* options) {
    struct vtrs::swapchain_support_bundle support_bundle = querySwapchainSupport_(surface);

    m_imageFormat = support_bundle.surfaceFormats.at(0).format;
    m_imageColors = support_bundle.surfaceFormats.at(0).colorSpace;

    for (auto& this_format : support_bundle.surfaceFormats) {
        if (this_format.format == VK_FORMAT_B8G8R8A8_SRGB && this_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            m_imageFormat = this_format.format;
            m_imageColors = this_format.colorSpace;
            break;
        }
    }

    m_imageExtend.width = support_bundle.surfaceCaps.currentExtent.width;
    m_imageExtend.height = support_bundle.surfaceCaps.currentExtent.height;

    if (support_bundle.surfaceCaps.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        m_imageExtend.width = 800;
        m_imageExtend.height = 600;
    }

    VkSwapchainCreateInfoKHR swapchain_info {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};

    swapchain_info.surface = surface;
    swapchain_info.minImageCount = support_bundle.surfaceCaps.minImageCount + 1;

    if (support_bundle.surfaceCaps.maxImageCount > 0 && swapchain_info.minImageCount > support_bundle.surfaceCaps.maxImageCount)
        swapchain_info.minImageCount = support_bundle.surfaceCaps.maxImageCount;

    swapchain_info.imageFormat = m_imageFormat;
    swapchain_info.imageColorSpace = m_imageColors;

    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    /* The imageArrayLayers specifies the amount of layers each
     * image consists of. This is always 1 unless we are developing
     * a stereoscopic 3D application.  */
    swapchain_info.imageArrayLayers = 1;

    if (options->graphicsQueueFamily.value() == options->surfaceQueueFamily.value()) {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;

    } else {
        uint32_t family_indices[] = {options->surfaceQueueFamily.value(), options->graphicsQueueFamily.value()};

        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = family_indices;
    }

    swapchain_info.preTransform = support_bundle.surfaceCaps.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = static_cast<VkPresentModeKHR>(options->presenterMode);
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = m_swapchain;
    swapchain_info.imageExtent = m_imageExtend;

    support_bundle.surfaceFormats.clear();
    support_bundle.presentModes.clear();

    auto result = vkCreateSwapchainKHR(m_logicalDevice, &swapchain_info, nullptr, &m_swapchain);
    VTRS_ASSERT_VK_RESULT(result, "Presentation infrastructure failed while creating swapchains.")
}

void vtrs::SurfacePresenter::obtainSwapViews_() {
    uint32_t image_count;
    auto result = vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &image_count, nullptr);
    VTRS_ASSERT_VK_RESULT(result, "Unable to obtain swap chain images.")

    m_imageChain.resize(image_count);
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &image_count, m_imageChain.data());

    m_viewsChain.resize(m_imageChain.size());

    for (size_t index = 0; index < m_imageChain.size(); index++) {
        VkImageViewCreateInfo image_view_info {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        image_view_info.image = m_imageChain.at(index);
        image_view_info.format = m_imageFormat;
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

        result = vkCreateImageView(m_logicalDevice, &image_view_info, nullptr, &(m_viewsChain.at(index)));
        VTRS_ASSERT_VK_RESULT(result, "Unable to create image view for image.")
    }
}

void vtrs::SurfacePresenter::bootstrap_(VkSurfaceKHR surface, vtrs::surface_presenter_opts* options) {
    createSwapchain_(surface, options);
    obtainSwapViews_();
}

vtrs::SurfacePresenter::SurfacePresenter(VkPhysicalDevice physical_device, VkDevice logical_device) :
        m_physicalDevice(physical_device),
        m_logicalDevice(logical_device) {
}

vtrs::SurfacePresenter*
vtrs::SurfacePresenter::factory(
        VkPhysicalDevice physical_device,
        VkDevice logical_device,
        vtrs::WindowSurface* surface,
        SurfacePresenter::Options* options) {

    auto presenter = new SurfacePresenter(physical_device, logical_device);
    presenter->bootstrap_(surface->getSurfaceHandle(), options);

    return presenter;
}

vtrs::SurfacePresenter::~SurfacePresenter() {
    vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);

    for (auto image_view : m_viewsChain) {
        vkDestroyImageView(m_logicalDevice, image_view, nullptr);
    }

    m_viewsChain.clear();
    m_imageChain.clear();
}

/**
 * surface_presenter.hpp - Infrastructure for presenting on window surface.
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

#pragma once

#include <vector>
#include <optional>
#include "vulkan_api.hpp"
#include "window_surface.hpp"

namespace vtrs {

struct surface_presenter_opts {
    uint32_t presenterMode = VK_PRESENT_MODE_FIFO_KHR;
    std::optional<uint32_t> surfaceQueueFamily;
    std::optional<uint32_t> graphicsQueueFamily;
};

struct swapchain_support_bundle {
    VkSurfaceCapabilitiesKHR surfaceCaps;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

class SurfacePresenter {

private:
    VkPhysicalDevice    m_physicalDevice = VK_NULL_HANDLE;
    VkDevice            m_logicalDevice = VK_NULL_HANDLE;
    VkSwapchainKHR      m_swapchain = VK_NULL_HANDLE;

    VkColorSpaceKHR m_imageColors = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkFormat        m_imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkExtent2D      m_imageExtend {};

    std::vector<VkImage>        m_imageChain {};
    std::vector<VkImageView>    m_viewsChain {};

    /**
     * @brief Retrieves swapchain support details before creating a swapchain.
     * @throw vtrs::RendererError Thrown if querying fails.
     *
     * There are basically three kinds of properties we need to check:
     * - Min/max number of images in swapchain.
     * - Min/max width and height of images.
     * - Surface formats (pixel format, color space).
     * - Available presentation modes.
     */
    struct swapchain_support_bundle querySwapchainSupport_(VkSurfaceKHR);

    /**
     * @brief Creates a swapchain with the specified options.
     * @param options Surface presenter configuration.
     */
    void createSwapchain_(VkSurfaceKHR, struct surface_presenter_opts* options);

    /**
     * @brief Retrieves images and image views from a swapchain.
     *
     * This method should be called only after creating swapchain.
     */
    void obtainSwapViews_();

    /**
     * @brief Bootstraps the surface presenter instance.
     * @param options Surface presenter configuration.
     *
     * The boostrap method will:
     * - Create a Vulkan swapchain.
     * - Retrieve the images and image views from the swapchain.
     */
    void bootstrap_(VkSurfaceKHR, struct surface_presenter_opts*);

    /**
     * @brief Initialises member variables.
     * @param physicalDevice Vulkan physical device handle.
     * @param logicalDevice Vulkan logical device handle.
     */
    explicit SurfacePresenter(VkPhysicalDevice, VkDevice);

public:
    typedef struct surface_presenter_opts Options;

    /**
     * @brief Creates and returns a new instance.
     * @param physical_device   Vulkan physical device handle.
     * @param logical_device    Vulkan logical device handle.
     * @param surface           Window surface for presenting.
     * @param options           Surface presenter configuration.
     * @return Instance of surface presenter.
     * @throws vtrs::RendererError Thrown if the factory method fails.
     */
    static SurfacePresenter* factory(VkPhysicalDevice, VkDevice, WindowSurface*, SurfacePresenter::Options*);

    /**
     * @brief Cleans up when an instance is destroyed.
     */
    ~SurfacePresenter();
};

} // namespace vtrs
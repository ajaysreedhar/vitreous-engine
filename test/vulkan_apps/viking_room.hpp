/**
 * viking-room.hpp - Vitreous Engine [test-vulkan-apps]
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

#pragma once
#define VK_USE_PLATFORM_XCB_KHR 1

#include <vector>
#include <optional>
#include <vulkan/vulkan.h>

#include "platform/xcb_client.hpp"

namespace vtest {

struct queue_family_indices {
    std::optional<uint32_t> graphicsIndex;
    std::optional<uint32_t> surfaceIndex;
};

struct queue_wrapper {
    VkQueue surfaceQueue;
    VkQueue graphicsQueue;
};

struct swapchain_support {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

typedef struct queue_family_indices QueueFamilyIndices;
typedef struct queue_wrapper QueueWrapper;
typedef struct swapchain_support SwapchainSupport;

/**
 * Vulkan Demo VikingRoom.
 *
 * @see [Vulkan Tutorial](https://vulkan-tutorial.com/Introduction)
 */
class VikingRoom {

private:
    static bool s_isInitialised;

    /**
     * @brief Holds supported instance extensions.
     */
    static std::vector<std::string> s_iExtensions;

    /**
     * @brief Holds supported GPU device extensions.
     */
    std::vector<std::string> m_gExtensions;

    std::vector<VkImage> m_swapImages;
    std::vector<VkImageView> m_swapImageViews;

    VkInstance m_instance;
    VkPhysicalDevice m_gpu;
    VkDevice m_device;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    VkFormat m_imageFormat;
    VkExtent2D m_extend2D;

    QueueWrapper m_queues;

    /**
     * Finds the GPU usability score after evaluating device properties.
     *
     * @param device An instance of VkPhysicalDevice
     * @return The calculated rating
     */
    static unsigned int findGPUScore_(VkPhysicalDevice device);

    static void enumerateExtensions_();

    void abortBootstrap_();

    void enumerateGPUExtensions_(VkPhysicalDevice);

    /**
     * Initialises Vulkan instance.
     *
     * This method will set the m_instance member variable.
     */
    void initVulkan_(std::vector<const char*>&);

    /**
     * Initialises physical GPU.
     *
     * This method will select a suitable GPU from the list of available
     * GPUs based on their score and set the m_gpu member variable.
     */
    void initGPU_();

    void initDevice_(vtest::QueueFamilyIndices);

    SwapchainSupport querySwapchainCapabilities_();

    void createSwapchain_(QueueFamilyIndices);

    QueueFamilyIndices findQueueFamilies_();

    void prepareSurface_(vtrs::XCBConnection*, uint32_t);

    void createImageViews_();

    void bootstrap_();

    explicit VikingRoom(std::vector<const char*>&);

public:
    static VikingRoom* factory(vtrs::XCBConnection*, uint32_t);
    static void printIExtensions();

    ~VikingRoom();

    /**
     * Prints the properties of the selected GPU.
     */
    void printGPUInfo();

    void printGPUExtensions();
};

} // namespace vtest

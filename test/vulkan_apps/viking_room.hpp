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
#define VK_USE_PLATFORM_WAYLAND_KHR 1

#include <vector>
#include <optional>
#include <vulkan/vulkan.h>

#include "platform/linux/xcb_client.hpp"

#define ASSERT_VK_RESULT(result, message) \
if (result != VK_SUCCESS) { \
    throw vtrs::RuntimeError(message, vtrs::RuntimeError::E_TYPE_VK_RESULT, result); \
}

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

struct spirv_code {
    char* data = nullptr;
    size_t size = 0;

    ~spirv_code() {
        free(data);
        size = 0;
    }
};

typedef struct queue_family_indices QueueFamilyIndices;
typedef struct queue_wrapper QueueWrapper;
typedef struct swapchain_support SwapchainSupport;
typedef struct spirv_code SPIRVCode;

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
    std::vector<VkFramebuffer> m_swapFramebuffers;

    VkInstance m_instance;
    VkPhysicalDevice m_gpu;
    VkDevice m_device;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    VkFormat m_imageFormat;
    VkExtent2D m_extend2D;
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    VkCommandPool m_commandPool;
    VkCommandBuffer m_commandBuffer;

    VkSemaphore m_imageAvailableSem;
    VkSemaphore m_rendererFinishedSem;
    VkFence m_inFlightFence;

    QueueWrapper m_queues;

    /**
     * Finds the GPU usability score after evaluating device properties.
     *
     * @param device An instance of VkPhysicalDevice
     * @return The calculated rating
     */
    static unsigned int findGPUScore_(VkPhysicalDevice device);

    static void enumerateExtensions_();

    static SPIRVCode* readSPIRVFile_(const std::string& abs_path);

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

    VkShaderModule createShaderModule_(const SPIRVCode*);

    void createRenderPass_();

    void createGraphicsPipeline_();

    void createFrameBuffers_();

    void createCommandPool_(QueueFamilyIndices);
    void createCommandBuffer_();
    void recordCommandBuffer_(VkCommandBuffer, uint32_t);
    void createSyncObjects_();

    void bootstrap_();

    explicit VikingRoom(std::vector<const char*>&);

public:
    static VikingRoom* factory(vtrs::XCBConnection*, uint32_t);

    static void printIExtensions();

    ~VikingRoom();

    void drawFrame();

    /**
     * Prints the properties of the selected GPU.
     */
    void printGPUInfo();

    void printGPUExtensions();
};

} // namespace vtest

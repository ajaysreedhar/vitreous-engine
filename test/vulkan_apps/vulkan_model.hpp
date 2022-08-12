/**
 * vulkan_model.hpp - Vitreous model test application declaration.
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

#include <optional>
#include "platform/linux/xcb_client.hpp"
#include "renderer/renderer_context.hpp"

namespace vtest {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> surfaceFamily;
};

struct SwapchainSupportBundle {
    VkSurfaceCapabilitiesKHR surfaceCaps;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct SPIRVBytes {
    char* data;
    size_t size;
};

/**
 * @brief An application to test Vulkan support and rendering capabilities.
 *
 * This implementation is loosely dependent on the renderer sub-system.
 * Most of the required objects are provided raw directly from Vulkan SDK.
 */
class VulkanModel {

private: // *** Private members *** //
    struct QueueFamilyIndices m_familyIndices {};
    vtrs::GPUDevice* m_gpu;

    VkDevice m_device = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    VkQueue m_surfaceQueue = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkExtent2D m_swapExtend {};
    VkFormat m_swapFormat = VK_FORMAT_B8G8R8A8_SRGB;
    std::vector<VkImage> m_swapImages;
    std::vector<VkImageView> m_swapViews;

    /**
     * @brief Finds the discrete GPU from the enumerated list of GPUs.
     * @return A handle to the discrete GPU.
     */
    static vtrs::GPUDevice* findDiscreteGPU_();

    /**
     * @brief Reads characters from a compiled SPIR-V shader file.
     * @param path The absolute path to the file.
     * @return The bytes read.
     */
    static struct SPIRVBytes readSPIRVShader(const std::string& path);

    /**
     * @brief Creates a XCB window surface.
     *
     * Note that this method should be called only is the os is Linux.
     */
    void createSurface_(vtrs::XCBConnection*, uint32_t);

    /**
     * @brief Creates logical device.
     *
     * his method will create logical device required for this application
     * and assigns handles to surface queue and graphics queue members.
     */
    void createLogicalDevice_();

    /**
     * @brief Creates swapchain.
     */
    void createSwapchain_();

    /**
     * @brief Creates image views for every swapchain image.
     */
    void createImageViews_();

    /**
     * @brief Creates a new shader module from the SPIR-V bytes.
     * @return Vulkan shader module.
     */
    VkShaderModule newShaderModule(struct SPIRVBytes);

    /**
     * @brief Sets up the graphics pipeline.
     */
    void setupGraphicsPipeline_();

    /**
     * Bootstraps the application.
     */
    void bootstrap_();

    /**
     * @brief Initialises the instance.
     * @param client An instance of XCB window client.
     *
     * This will also initialise a renderer context
     * if not already initialised. New objects can be
     * created only by calling factory static method.
     */
    VulkanModel();

public: // *** Public members *** //
    static VulkanModel* factory(vtrs::XCBClient*, uint32_t);

    /**
     * @brief Cleans up upon destruction of the object.
     */
    ~VulkanModel();

    /**
     * @brief Prints the selected GPU information.
     *
     * This method simply invokes the printInfo
     * method in the selected GPU instance.
     */
    void printGPUInfo();
};

} // namespace vtest
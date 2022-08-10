/**
 * renderer_context.hpp - Vulkan renderer context abstraction.
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
#include <map>
#include "vulkan_api.hpp"
#include "gpu_device.hpp"

namespace vtrs {

/**
 * @brief Renderer context abstracts the Vulkan instance.
 *
 * This class wraps a VkInstance object. Generally, only one
 * instance is required per application for their entire run.
 */
class RendererContext {

private:
    static inline bool s_isInitialised = false;
    static inline VkInstance s_instance {};
    static inline std::map<uint32_t, GPUDevice*> s_gpuList {};

    /**
     * @brief Creates the Vulkan context.
     */
    static void initVulkan_(std::vector<const char*>&);

    /**
     * @brief Enumerates GPUs available in the system.
     */
    static void enumerateGPUs_();

public:

    /**
     * @brief Initialises the renderer context.
     * @throws RendererError Thrown if the context is already initialised.
     *
     * This method initialises the Vulkan instance, enumerates the list of
     * available physical GPUs and their capabilities.
     *
     * Once called, the s_isInitialised member is set to true to prevent
     * further initialisation. An exception is thrown if called again.
     */
    static void initialise();

    /**
     * @brief Destroys the renderer context.
     * @throws RendererError Thrown if the context is not initialised.
     *
     * This method destroys the vulkan instance and releases memory.
     */
    static void destroy();

    /**
     * @brief Returns the enumerated list of GPUs.
     * @return A vector containing the GPUDevice instances.
     */
    static std::vector<GPUDevice*> getGPUList();

    /**
     * @brief Returns the Global Vulkan instance handle.
     * @return Vulkan instance handle.
     */
    static VkInstance getInstanceHandle();
};

} // namespace vtrs

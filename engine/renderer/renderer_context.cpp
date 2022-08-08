/**
 * renderer_factory.cpp - Vulkan renderer context definitions.
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

#include "renderer_context.hpp"
#include "assert.hpp"

/**
 * @brief Creates the Vulkan context.
 */
void vtrs::RendererContext::initVulkan_(std::vector<const char*>& extensions) {
    VkApplicationInfo app_info {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName = "Vitreous Renderer";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Vitreous-Engine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_info {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = extensions.size();
    instance_info.ppEnabledExtensionNames = extensions.data();

#if defined(VTRS_MODE_DEBUG) && VTRS_MODE_DEBUG == 1
    std::vector<const char*> layers({
        "VK_LAYER_KHRONOS_validation"
    });

    instance_info.enabledLayerCount = layers.size();
    instance_info.ppEnabledLayerNames = layers.data();
#endif

    auto result = vkCreateInstance(&instance_info, nullptr, &s_instance);
    VTRS_ASSERT_VK_RESULT(result, "Unable to initialise renderer context.")
}

/**
 * @brief Enumerates GPUs available in the system.
 * @throws vtrs::RendererError Thrown if GPU enumeration fails.
 */
void vtrs::RendererContext::enumerateGPUs_() {
    std::vector<GPUDevice*> devices = GPUDevice::enumerate(s_instance);

    for (auto gpu : devices) {
        uint32_t device_id = gpu->getDeviceId();
        s_gpuList.insert(std::make_pair(device_id, gpu));
    }
}

/**
 * @brief Initialises the renderer context.
 * @throws RendererError Thrown if the context is already initialised.
 *
 * This method initialises the Vulkan instance, enumerates the list of
 * available physical GPUs and their capabilities.
 */
void vtrs::RendererContext::initialise() {
    if (s_isInitialised) {
        throw RendererError("Renderer context is already initialised!", RendererError::E_TYPE_GENERAL);
    }

    std::vector<const char*> extensions {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    };

    initVulkan_(extensions);
    s_isInitialised = true;

    enumerateGPUs_();
}

/**
 * @brief Destroys the renderer context.
 * @throws RendererError Thrown if the context is not initialised.
 *
 * This method destroys the vulkan instance and releases memory.
 */
void vtrs::RendererContext::destroy() {
    if (!s_isInitialised) {
        throw RendererError("Can not destroy the context without initialising!", RendererError::E_TYPE_GENERAL);
    }

    for(auto& gpu_pair : s_gpuList) {
        delete gpu_pair.second;
    }

    vkDestroyInstance(s_instance, nullptr);
}

/**
 * @brief Returns the enumerated list of GPUs.
 * @return A vector containing the GPUDevice instances.
 */
std::vector<vtrs::GPUDevice*> vtrs::RendererContext::getGPUList() {
    int index = 0;
    std::vector<vtrs::GPUDevice*> gpu_list(s_gpuList.size());

    for(auto gpu_pair : s_gpuList) {
        gpu_list.at(index) = gpu_pair.second;
        index++;
    }

    return gpu_list;
}

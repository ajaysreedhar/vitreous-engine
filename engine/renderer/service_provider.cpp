/**
 * service_provider.cpp - Provides various rendering services.
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

#include <set>
#include <cstring>
#include "except.hpp"
#include "assert.hpp"
#include "service_provider.hpp"

void vtrs::ServiceProvider::bootstrap_(vtrs::service_provider_opts* options) {
    float queue_priority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queue_info {};

    for (auto required_index : options->queueFamilyIndices) {
        VkDeviceQueueCreateInfo info {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        info.queueFamilyIndex = required_index;
        info.pQueuePriorities = &queue_priority;
        info.queueCount = 1;

        queue_info.push_back(info);
    }

    VkPhysicalDeviceFeatures gpu_features {};
    gpu_features.samplerAnisotropy = options->enableAnisotropy;

    std::vector<const char*> req_extensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo device_info {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.pQueueCreateInfos = queue_info.data();
    device_info.queueCreateInfoCount = queue_info.size();
    device_info.ppEnabledExtensionNames = req_extensions.data();
    device_info.enabledExtensionCount = req_extensions.size();
    device_info.pEnabledFeatures = &gpu_features;

    auto result = vkCreateDevice(m_rendererGPU->getDeviceHandle(), &device_info, nullptr, &m_logicalDevice);
    VTRS_ASSERT_VK_RESULT(result, "Could not bootstrap service provider.")
}

vtrs::ServiceProvider::ServiceProvider(RendererGPU* renderer_gpu) : m_rendererGPU(renderer_gpu) {

}

vtrs::ServiceProvider::~ServiceProvider() {
    vkDestroyDevice(m_logicalDevice, nullptr);
}

vtrs::ServiceProvider *vtrs::ServiceProvider::from(vtrs::RendererGPU* hardware, vtrs::ServiceProvider::Options* options) {
    auto provider = new ServiceProvider(hardware);
    provider->bootstrap_(options);

    return provider;
}

vtrs::SurfacePresenter* vtrs::ServiceProvider::createSurfacePresenter(vtrs::WindowSurface* surface) {
    /* Checking if required extensions for swapchain are supported by the GPU. */
    int extension_flag = 0;
    std::vector<const char*> extension_names = m_rendererGPU->getExtensionNames();

    for(auto& name : extension_names) {
        if (strcmp(name, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            extension_flag++;
            break;
        }
    }

    extension_names.clear();

    if (extension_flag <= 0) {
        throw vtrs::RendererError("GPU does not provide presentation support.", vtrs::RendererError::E_TYPE_GENERAL);
    }

    vtrs::SurfacePresenter::Options options {};
    options.graphicsQueueFamily = m_rendererGPU->getQueueFamilyIndex(vtrs::RendererGPU::QUEUE_FAMILY_INDEX_GRAPHICS);
    options.surfaceQueueFamily = m_rendererGPU->getQueueFamilyIndex(surface->getSurfaceHandle());

    return vtrs::SurfacePresenter::factory(m_rendererGPU->getDeviceHandle(), m_logicalDevice, surface, &options);
}

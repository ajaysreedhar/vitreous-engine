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

void vtest::VulkanModel::createSurface_(vtrs::XCBConnection* connection, uint32_t window) {
    VkXcbSurfaceCreateInfoKHR surface_info {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
    surface_info.connection = connection;
    surface_info.window = window;

    auto result = vkCreateXcbSurfaceKHR(vtrs::RendererContext::getInstanceHandle(), &surface_info, nullptr, &m_surface);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create XCB surface.")
}

void vtest::VulkanModel::createLogicalDevice_() {
    float queue_priority = 1.0f;
    VkPhysicalDeviceFeatures gpu_features {};

    std::vector<VkDeviceQueueCreateInfo> queue_info {};
    std::set<uint32_t> required_indices {m_familyIndices.graphicsFamily.value(), m_familyIndices.surfaceFamily.value()};

    for (auto required_index : required_indices) {
        VkDeviceQueueCreateInfo info {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        info.queueFamilyIndex = required_index;
        info.pQueuePriorities = &queue_priority;
        info.queueCount = 1;

        queue_info.push_back(info);
    }

    VkDeviceCreateInfo device_info {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.pQueueCreateInfos = queue_info.data();
    device_info.queueCreateInfoCount = queue_info.size();
    device_info.pEnabledFeatures = &gpu_features;

    auto result = vkCreateDevice(m_gpu->getDeviceHandle(), &device_info, nullptr, &m_device);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create logical device.")

    vkGetDeviceQueue(m_device, m_familyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_familyIndices.surfaceFamily.value(), 0, &m_surfaceQueue);
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

    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(vtrs::RendererContext::getInstanceHandle(), m_surface, nullptr);

    m_gpu = nullptr;
}

void vtest::VulkanModel::printGPUInfo() {
    m_gpu->printInfo();
}

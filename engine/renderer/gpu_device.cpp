/**
 * gpu_device.cpp - Contains necessary definitions for representing a GPU.
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
#include <cstdlib>
#include "platform/logger.hpp"
#include "gpu_device.hpp"
#include "assert.hpp"

uint32_t vtrs::GPUDevice::recordCapabilities_() {
    uint32_t score = 0;

    vkGetPhysicalDeviceProperties(m_device, m_properties);
    vkGetPhysicalDeviceFeatures(m_device, m_features);

    switch (m_properties->deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score = score + 1000;
            break;

        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score = score + 750;
            break;

        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score = score + 500;
            break;

        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score = score + 250;
            break;

        default:
            break;
    }

    return score + m_properties->limits.maxImageDimension2D;
}

void vtrs::GPUDevice::mapQueueFamilies_() {
    uint32_t family_count = 0, family_index = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &family_count, nullptr);

    std::vector<VkQueueFamilyProperties> family_list(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &family_count, family_list.data());

    for (auto& family : family_list) {

        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            m_qFamilyIndices.insert(std::make_pair(GPUDevice::QUEUE_FAMILY_INDEX_GRAPHICS, family_index));

        if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
            m_qFamilyIndices.insert(std::make_pair(GPUDevice::QUEUE_FAMILY_INDEX_COMPUTE, family_index));

        if (family.queueFlags & VK_QUEUE_TRANSFER_BIT)
            m_qFamilyIndices.insert(std::make_pair(GPUDevice::QUEUE_FAMILY_INDEX_TRANSFER, family_index));

        if (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
            m_qFamilyIndices.insert(std::make_pair(GPUDevice::QUEUE_FAMILY_INDEX_SPARSE_B, family_index));

        if (family.queueFlags & VK_QUEUE_PROTECTED_BIT)
            m_qFamilyIndices.insert(std::make_pair(GPUDevice::QUEUE_FAMILY_INDEX_PROTECTED, family_index));

        family_index++;
    }
}

std::vector<vtrs::GPUDevice*> vtrs::GPUDevice::enumerate(VkInstance instance) {
    uint32_t gpu_count;

    auto result = vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
    VTRS_ASSERT_VK_RESULT(result, "Unable to query installed GPUs.")

    if (gpu_count <= 0) {
        throw RendererError("GPUs with Vulkan API support is required to run.", RendererError::E_TYPE_GENERAL);
    }

    int device_index = 0;
    std::vector<VkPhysicalDevice> candidates(gpu_count);
    std::vector<GPUDevice*> device_list(gpu_count);

    result = vkEnumeratePhysicalDevices(instance, &gpu_count, candidates.data());
    VTRS_ASSERT_VK_RESULT(result, "Unable to query GPUs with Vulkan support.")

    for(auto gpu : candidates) {
        auto device = new GPUDevice(gpu);
        device->mapQueueFamilies_();

        device_list.at(device_index) = device;
        device_index++;
    }

    return device_list;
}

vtrs::GPUDevice::GPUDevice(VkPhysicalDevice device) {
    m_properties = new VkPhysicalDeviceProperties();
    m_features = new VkPhysicalDeviceFeatures();
    m_device = device;

    m_gpuScore = recordCapabilities_();
}

vtrs::GPUDevice::~GPUDevice() {
#if (defined(VTRS_MODE_DEBUG) && VTRS_MODE_DEBUG == 1)
    vtrs::Logger::info("Cleaning up GPU information", m_properties->deviceID, m_properties->deviceName);
#endif

    free(m_properties);
    free(m_features);

    m_qFamilyIndices.clear();
}

uint32_t vtrs::GPUDevice::getDeviceId() const {
    return m_properties->deviceID;
}

uint32_t vtrs::GPUDevice::getGPUScore() const {
    return m_gpuScore;
}

void vtrs::GPUDevice::printInfo() {
    const char* type;

    switch (m_properties->deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            type = "Discrete GPU";
            break;

        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            type = "Integrated GPU";
            break;

        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            type = "CPU";
            break;

        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            type = "Virtual GPU";
            break;

        default:
            type = "Unknown";
    }

    vtrs::Logger::print("Enumerated GPU Information");
    vtrs::Logger::print("**************************");
    vtrs::Logger::print("Device Id:", m_properties->deviceID);
    vtrs::Logger::print("Vendor Id:", m_properties->vendorID);
    vtrs::Logger::print("Device Name:", m_properties->deviceName);
    vtrs::Logger::print("Device Type:", type);
    vtrs::Logger::print("GPU Score:", m_gpuScore);
    vtrs::Logger::print("API Version:", m_properties->apiVersion);
    vtrs::Logger::print("");
}

/**
 * demo-app.cpp - Vitreous Engine [test-vulkan-apps]
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

#include <vector>
#include <map>
#include "engine/except/runtime-error.hpp"
#include "engine/platform/logger.hpp"
#include "viking-room.hpp"

/**
 * Static utility function definitions.
 *
 * ========================================================================
 */

/**
 * Finds the GPU usability score after evaluating device properties.
 *
 * @param device An instance of VkPhysicalDevice
 * @return The calculated score
 */
unsigned int vtest::VikingRoom::findGPUScore_(VkPhysicalDevice device) {
    unsigned int score = 0;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    switch (properties.deviceType) {
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

    score = score + properties.limits.maxImageDimension2D;

    return score;
}

/**
 * Private member function definitions.
 *
 * ========================================================================
 */

/**
 * Initialises Vulkan instance.
 *
 * This method will set the m_instance member variable.
 */
void vtest::VikingRoom::initVulkan_() {
    VkApplicationInfo app_info {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vulkan Demo VikingRoom";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Vitreous Engine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    auto result = vkCreateInstance(&create_info, nullptr, &m_instance);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create Vulkan instance", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }
}

/**
 * Initialises physical GPU.
 *
 * This method will select a suitable GPU from the list of available
 * GPUs based on their score and set the m_physicalGPU member variable.
 */
void vtest::VikingRoom::initPhysicalGPU_() {
    uint32_t gpu_count = 0;

    auto result = vkEnumeratePhysicalDevices(m_instance, &gpu_count, nullptr);

    if (result != VK_SUCCESS || gpu_count == 0) {
        throw vtrs::RuntimeError("Unable to find any GPUs with Vulkan support.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    std::vector<VkPhysicalDevice> gpu_list(gpu_count);
    vkEnumeratePhysicalDevices(m_instance, &gpu_count, gpu_list.data());

    std::multimap<unsigned int, VkPhysicalDevice> candidates;

    for(VkPhysicalDevice& gpu : gpu_list) {
        unsigned int score = vtest::VikingRoom::findGPUScore_(gpu);
        candidates.insert(std::make_pair(score, gpu));
    }

    std::reverse_iterator selected = candidates.rbegin();

    if (selected->first == 0) {
        throw vtrs::RuntimeError("Unable to select a suitable GPU.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    m_physicalGPU = selected->second;
}

vtest::QueueFamilyIndices vtest::VikingRoom::findQueueFamilies_() {
    uint32_t family_count = 0, family_index = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalGPU, &family_count, nullptr);

    std::vector<VkQueueFamilyProperties> family_list(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalGPU, &family_count, family_list.data());

    vtest::QueueFamilyIndices indices;

    for (auto& family : family_list) {
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = family_index;
        }

        family_index++;
    }

    return indices;
}

void vtest::VikingRoom::initLogicalDevice_(vtest::QueueFamilyIndices indices) {
    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = indices.graphicsFamily.value(),
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo device_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .pEnabledFeatures = &deviceFeatures
    };

    auto result = vkCreateDevice(m_physicalGPU, &device_info, nullptr, &m_logicalDevice);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create logical device.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }
}

/**
 * Public member function definitions.
 *
 * ========================================================================
 */

/**
 * Initialises the instance.
 *
 * Vulkan instance and GPU are initialised here.
 */
vtest::VikingRoom::VikingRoom() :
        m_instance{},
        m_logicalDevice{},
        m_physicalGPU(VK_NULL_HANDLE) {
    initVulkan_();
    initPhysicalGPU_();

    vtest::QueueFamilyIndices indices = findQueueFamilies_();

    if (!indices.graphicsFamily.has_value()) {
        throw vtrs::RuntimeError("Unable to obtain required queue families.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    initLogicalDevice_(indices);
}

/**
 * Cleans up.
 */
vtest::VikingRoom::~VikingRoom() {
    vkDestroyDevice(m_logicalDevice, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

/**
 * Prints the properties of the selected GPU.
 */
void vtest::VikingRoom::printGPUInfo() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_physicalGPU, &properties);

    const char* type;

    switch (properties.deviceType) {
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

    vtrs::Logger::print("Selected GPU Information");
    vtrs::Logger::print("************************");
    vtrs::Logger::print("Device Id:", properties.deviceID);
    vtrs::Logger::print("Vendor Id:", properties.vendorID);
    vtrs::Logger::print("Device Name:", properties.deviceName);
    vtrs::Logger::print("Device Type:", type);
    vtrs::Logger::print("API Version:", properties.apiVersion);
}

void vtest::VikingRoom::createSurface_() {

}

/**
 * viking-room.cpp - Vitreous Engine [test-vulkan-apps]
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
#include <set>
#include "engine/except/runtime-error.hpp"
#include "engine/platform/logger.hpp"
#include "viking-room.hpp"

bool vtest::VikingRoom::s_isInitialised = false;
std::vector<std::string> vtest::VikingRoom::s_iExtensions{};

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

void vtest::VikingRoom::enumerateExtensions_() {
    uint32_t count = 0;
    auto result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Failed to query instance extensions.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    s_iExtensions.reserve(count);

    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());

    for (auto& item : extensions) {
        s_iExtensions.emplace_back(item.extensionName);
    }
}

/**
 * Private member function definitions.
 *
 * ========================================================================
 */
void vtest::VikingRoom::abortBootstrap_() {
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

/**
 * Initialises Vulkan instance.
 *
 * This method will set the m_instance member variable.
 */
void vtest::VikingRoom::initVulkan_(std::vector<const char*>& extensions) {
    VkApplicationInfo app_info {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vulkan Demo VikingRoom";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Vitreous Engine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> validation_layers({
        "VK_LAYER_KHRONOS_validation"
    });

    VkInstanceCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = extensions.size();
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();

    auto result = vkCreateInstance(&create_info, nullptr, &m_instance);

    if (result != VK_SUCCESS) {
        throw vtrs::RuntimeError("Unable to create Vulkan instance", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }
}

void vtest::VikingRoom::enumerateGPUExtensions_(VkPhysicalDevice device) {
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

    for (const auto& extension : extensions) {
        m_gExtensions.emplace_back(std::string(extension.extensionName));
    }
}


/**
 * Initialises physical GPU.
 *
 * This method will select a suitable GPU from the list of available
 * GPUs based on their score and set the m_gpu member variable.
 */
void vtest::VikingRoom::initGPU_() {
    uint32_t gpu_count = 0;

    auto result = vkEnumeratePhysicalDevices(m_instance, &gpu_count, nullptr);

    if (result != VK_SUCCESS || gpu_count == 0) {
        vkDestroyInstance(m_instance, nullptr);
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
        vkDestroyInstance(m_instance, nullptr);
        throw vtrs::RuntimeError("Unable to select a suitable GPU.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    this->enumerateGPUExtensions_(selected->second);
    m_gpu = selected->second;
}

vtest::QueueFamilyIndices vtest::VikingRoom::findQueueFamilies_() {
    uint32_t family_count = 0, family_index = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &family_count, nullptr);

    std::vector<VkQueueFamilyProperties> family_list(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &family_count, family_list.data());

    VkBool32 surfaceSupport;
    vtest::QueueFamilyIndices indices;

    for (auto& family : family_list) {
        vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, family_index, m_surface, &surfaceSupport);

        if (surfaceSupport >= 1){
            indices.surfaceIndex = family_index;
        }

        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsIndex = family_index;
        }

        if (indices.graphicsIndex.has_value() && indices.surfaceIndex.has_value()) {
            break;
        }

        family_index++;
    }

    return indices;
}

void vtest::VikingRoom::initDevice_(vtest::QueueFamilyIndices indices) {
    float queue_priority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    std::set<uint32_t> index_set = {indices.graphicsIndex.value(), indices.surfaceIndex.value()};

    for(uint32_t index : index_set) {
        VkDeviceQueueCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };

        queue_infos.push_back(create_info);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<const char*> extensions({
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    });

    VkDeviceCreateInfo device_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size()),
        .pQueueCreateInfos = queue_infos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

    auto result = vkCreateDevice(m_gpu, &device_info, nullptr, &m_device);

    if (result != VK_SUCCESS) {
        abortBootstrap_();
        throw vtrs::RuntimeError("Unable to create logical device.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }

    vkGetDeviceQueue(m_device, indices.graphicsIndex.value(), 0, &(m_queues.graphicsQueue));
    vkGetDeviceQueue(m_device, indices.surfaceIndex.value(), 0, &(m_queues.surfaceQueue));
}

void vtest::VikingRoom::bootstrap_() {
    vtest::QueueFamilyIndices indices = findQueueFamilies_();

    if (!indices.graphicsIndex.has_value() || !indices.surfaceIndex.has_value()) {
        abortBootstrap_();
        throw vtrs::RuntimeError("Unable to obtain required queue families.", vtrs::RuntimeError::E_TYPE_GENERAL);
    }

    initDevice_(indices);
}

void vtest::VikingRoom::prepareSurface_(vtest::XCBConnection* connection, uint32_t window) {
    VkXcbSurfaceCreateInfoKHR surface_info {};
    surface_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surface_info.connection = connection;
    surface_info.window = window;

    auto result = vkCreateXcbSurfaceKHR(m_instance, &surface_info, nullptr, &this->m_surface);

    if (result != VK_SUCCESS) {
        vkDestroyInstance(m_instance, nullptr);
        throw vtrs::RuntimeError("Unable to create XCB surface.", vtrs::RuntimeError::E_TYPE_VK_RESULT, result);
    }
}

/**
 * Initialises the instance.
 *
 * Vulkan instance and GPU are initialised here.
 */
vtest::VikingRoom::VikingRoom(std::vector<const char*>& extensions) :
        m_instance{},
        m_device{},
        m_surface{},
        m_queues{},
        m_gpu(VK_NULL_HANDLE) {
    initVulkan_(extensions);
    initGPU_();
}

/**
 * Public member function definitions.
 *
 * ========================================================================
 */

vtest::VikingRoom *vtest::VikingRoom::factory(vtest::XCBConnection* connection, uint32_t window) {
    if (!s_isInitialised) {
        enumerateExtensions_();
        s_isInitialised = true;
    }

    std::vector<const char*> extensions({
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    });

    auto instance = new VikingRoom(extensions);
    instance->prepareSurface_(connection, window);
    instance->bootstrap_();

    return instance;
}

/**
 * Cleans up.
 */
vtest::VikingRoom::~VikingRoom() {
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

/**
 * Prints the properties of the selected GPU.
 */
void vtest::VikingRoom::printGPUInfo() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_gpu, &properties);

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
    vtrs::Logger::print("");
}

void vtest::VikingRoom::printIExtensions() {
    vtrs::Logger::print("Available Instance Extensions");
    vtrs::Logger::print("************************");

    for (auto& name : s_iExtensions) {
        vtrs::Logger::print(name);
    }

    vtrs::Logger::print("");
}

void vtest::VikingRoom::printGPUExtensions() {
    vtrs::Logger::print("Available GPU Extensions");
    vtrs::Logger::print("************************");

    for (auto& name : m_gExtensions) {
        vtrs::Logger::print(name);
    }

    vtrs::Logger::print("");
}

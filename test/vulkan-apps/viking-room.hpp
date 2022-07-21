/**
 * demo-app.hpp - Vitreous Engine [test-vulkan-apps]
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

#include <optional>
#include <vulkan/vulkan.h>

namespace vtest {

struct queue_family_indices {
    std::optional<uint32_t> graphicsFamily;
};

typedef struct queue_family_indices QueueFamilyIndices;

/**
 * Vulkan Demo VikingRoom.
 *
 * @see [Vulkan Tutorial](https://vulkan-tutorial.com/Introduction)
 */
class VikingRoom {

private:
    VkInstance m_instance;
    VkPhysicalDevice m_physicalGPU;
    VkDevice m_logicalDevice;

    /**
     * Finds the GPU usability score after evaluating device properties.
     *
     * @param device An instance of VkPhysicalDevice
     * @return The calculated rating
     */
    static unsigned int findGPUScore_(VkPhysicalDevice device);

    /**
     * Initialises Vulkan instance.
     *
     * This method will set the m_instance member variable.
     */
    void initVulkan_();

    /**
     * Initialises physical GPU.
     *
     * This method will select a suitable GPU from the list of available
     * GPUs based on their score and set the m_physicalGPU member variable.
     */
    void initPhysicalGPU_();

    void initLogicalDevice_(vtest::QueueFamilyIndices);

    QueueFamilyIndices findQueueFamilies_();

public:
    VikingRoom();
    ~VikingRoom();

    /**
     * Prints the properties of the selected GPU.
     */
    void printGPUInfo();
};

} // namespace vtest

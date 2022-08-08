/**
 * gpu_device.hpp - Contains necessary declarations for representing a GPU.
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
#include <map>
#include "vulkan_api.hpp"

namespace vtrs {

/**
 * @brief Represents a physical GPU installed on the system.
 *
 * Stores information regarding the capabilities of the GPU
 * including its features, properties and queue families.
 */
class GPUDevice {

private:
    uint32_t m_gpuScore = 0;
    VkPhysicalDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties* m_properties;
    VkPhysicalDeviceFeatures* m_features;

    std::map<int, uint32_t> m_qFamilyIndices;

    /**
     * @brief Records the capabilities of the GPU.
     * @return A GPU score based on the capabilities.
     *
     * This method will query the device properties and features.
     * A score is calculated and returned based on the above parameters.
     */
    uint32_t recordCapabilities_();

    /**
     * @brief Finds queue families supported by this GPU.
     *
     * Maps queue family indices discovered in this GPU against their type.
     */
    void mapQueueFamilies_();

    /**
     * @brief Initialises the instance with device handle and capabilities.
     * @param device A Vulkan physical device handle.
     */
    explicit GPUDevice(VkPhysicalDevice device);

public:
    enum QueueFamilyType : int {
        QUEUE_FAMILY_INDEX_GRAPHICS = 15,
        QUEUE_FAMILY_INDEX_COMPUTE,
        QUEUE_FAMILY_INDEX_TRANSFER,
        QUEUE_FAMILY_INDEX_SPARSE_B,
        QUEUE_FAMILY_INDEX_PROTECTED
    };

    /**
     * @brief Enumerates the list of GPUs available on the machine.
     * @throw vtrs::RendererException Thrown if the enumeration fails.
     * @return A vector containing GPU device instances.
     */
    static std::vector<GPUDevice*> enumerate(VkInstance);

    /**
     * @brief Cleans up when an instance is destroyed.
     */
    ~GPUDevice();

    /**
     * @brief Returns the id of the GPU represented by current object.
     * @return The device id.
     */
    [[nodiscard]] uint32_t getDeviceId() const;

    /**
     * @brief Returns the score of the GPU represented by current object.
     * @return The GPU score.
     */
    [[nodiscard]] uint32_t getGPUScore() const;

    /**
     * @brief Returns the queue family index matched by requested type.
     * @return The queue family index.
     * @throws vtrs::RendererError Thrown if queue family type is nil.
     */
    [[nodiscard]] uint32_t getQueueFamilyIndex(QueueFamilyType) const;

    /**
     * @brief Prints the GPU information to the console.
     */
    void printInfo();
};

} // namespace vtrs
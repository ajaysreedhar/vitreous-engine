/**
 * service_provider.hpp - Provides various rendering services.
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

#include <set>
#include "vulkan_api.hpp"
#include "renderer_gpu.hpp"
#include "surface_presenter.hpp"

namespace vtrs {

struct service_provider_opts {
    std::set<uint32_t> queueFamilyIndices {};
    VkBool32 enableAnisotropy = VK_TRUE;
};

class ServiceProvider {

private:
    VkDevice            m_logicalDevice = VK_NULL_HANDLE;
    vtrs::RendererGPU*  m_rendererGPU = nullptr;

    /**
     * @brief Bootstraps the service provider.
     * @param options Service provider configuration.
     *
     * The boostrap method will:
     * - Create a Vulkan logical device.
     */
    void bootstrap_(struct service_provider_opts* options);

    /**
     * @brief Initialises member variables.
     * @param hardware A GPU on which the service provider will work.
     */
    explicit ServiceProvider(RendererGPU*);

public:
    typedef struct service_provider_opts Options;

    /**
     * @brief Cleans up when an instance is deleted.
     */
    ~ServiceProvider();

    /**
     * @brief Creates a new service provider from the selected GPU.
     * @param hardware A GPU on which the service provider will work.
     * @param options Service provider configuration.
     * @return An instance of the service provider class.
     */
    static ServiceProvider* from(RendererGPU*, ServiceProvider::Options*);

    SurfacePresenter* createSurfacePresenter(vtrs::WindowSurface* surface);
};

} // namespace vtrs
/**
 * window_surface.hpp - Cross platform window surface for rendering.
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

#include "platform/linux/xcb_client.hpp"
#include "platform/linux/wayland_client.hpp"
#include "vulkan_api.hpp"

namespace vtrs {

/**
 * @brief Represents a window surface for rendering.
 *
 * Window surfaces on Windows and Linux can be created
 * by calling the appropriate constructor.
 */
class WindowSurface {

private:
    VkSurfaceKHR m_surface;

public:
#if defined(VTRS_OS_TYPE_LINUX) && VTRS_OS_TYPE_LINUX == 1

    /**
     * @brief Creates a XCB surface on Linux machines.
     * @param connection Reference to the XCB connection
     * @param window Reference to a XCB window
     */
    explicit WindowSurface(vtrs::XCBConnection* connection, vtrs::XCBWindow* window);

    /**
     * @brief Creates Wayland surface on Linux machines.
     * @param connection Reference to the XCB connection
     * @param window Reference to a XCB window
     */
    explicit WindowSurface(vtrs::WaylandClient* client);
#endif

    /**
     * @brief Cleans up when an instance is destroyed.
     */
    ~WindowSurface();

    [[nodiscard]] VkSurfaceKHR getSurfaceHandle() const;
};

} // namespace vtrs
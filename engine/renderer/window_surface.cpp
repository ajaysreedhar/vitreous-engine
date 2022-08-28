/**
 * window_surface.cpp - Cross platform window surface for rendering.
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

#include "assert.hpp"
#include "renderer_context.hpp"
#include "window_surface.hpp"

#if defined(VTRS_OS_TYPE_LINUX) && VTRS_OS_TYPE_LINUX == 1
vtrs::WindowSurface::WindowSurface(vtrs::XCBConnection* connection, vtrs::XCBWindow* window) {
    VkXcbSurfaceCreateInfoKHR surface_info {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
    surface_info.connection = connection;
    surface_info.window = window->identifier;

    auto result = vkCreateXcbSurfaceKHR(vtrs::RendererContext::getInstanceHandle(), &surface_info, nullptr, &m_surface);
    VTRS_ASSERT_VK_RESULT(result, "Unable to create XCB surface.")
}
#endif

vtrs::WindowSurface::~WindowSurface() {
    vkDestroySurfaceKHR(vtrs::RendererContext::getInstanceHandle(), m_surface, nullptr);
}
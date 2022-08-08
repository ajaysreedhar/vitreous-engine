/**
 * vulkan_api.hpp - Configures Vulkan API headers depending on platforms.
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

#include "platform/standard.hpp"

/* IMPORTANT: This line should be put before including vulkan headers.
 * If OS is Linux, include support for Wayland and XCB surfaces. */
#if defined(VTRS_OS_TYPE_LINUX) && VTRS_OS_TYPE_LINUX == 1
#define VK_USE_PLATFORM_XCB_KHR 1
#define VK_USE_PLATFORM_WAYLAND_KHR 1
#endif

#include <vulkan/vulkan.h>

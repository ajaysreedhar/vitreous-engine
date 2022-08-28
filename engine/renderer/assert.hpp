/**
 * assert.hpp - Assertion macro utility header.
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

#include "vulkan_api.hpp"
#include "except.hpp"

#define VTRS_ASSERT_VK_RESULT(result, message) \
if (result != VK_SUCCESS) { \
    throw vtrs::RendererError(message, vtrs::RuntimeError::E_TYPE_VK_RESULT, result); \
}

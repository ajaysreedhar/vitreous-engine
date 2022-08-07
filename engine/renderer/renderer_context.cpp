/**
 * renderer_factory.cpp - Vulkan renderer context definitions.
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

#include "except.hpp"
#include "renderer_context.hpp"

/**
 * @brief Initialises the Vulkan renderer context.
 * @throws vtrs::RendererError If the context is already initialised.
 */
void vtrs::RendererContext::initialise() {
    if (s_isInitialised) {
        throw RendererError("Renderer context is already initialised!", RendererError::E_TYPE_GENERAL);
    }

    s_isInitialised = true;
}

/**
 * test-main.cpp - Vitreous Engine [test-vulkan-demo]
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

#include <cstdlib>
#include "engine/except/runtime-error.hpp"
#include "engine/platform/logger.hpp"
#include "application.hpp"

int main() {
    vtrs::Logger::info("Test: Vulkan Demo Application");

    vtest::Application* application;

    try {
        application = new vtest::Application();
        application->printGPUInfo();

    } catch (vtrs::RuntimeError& error) {
        vtrs::Logger::fatal(error.what(), "Kind:", error.getKind(), ", Code:", error.getCode());
        return EXIT_FAILURE;
    }

    delete application;
    return EXIT_SUCCESS;
}

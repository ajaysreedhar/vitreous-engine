/**
 * test-main.cpp - Vitreous Engine [test-vulkan-apps]
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
#include "xcb-client/xcb-window.hpp"
#include "viking-room.hpp"

int main() {
    vtrs::Logger::info("Test: Vulkan Demo VikingRoom");

    vtest::XCBWindow* window;
    vtest::VikingRoom* application;
    uint32_t w_id;

    try {
        window = new vtest::XCBWindow();
        w_id = window->createWindow(800, 600);

        application = new vtest::VikingRoom(window->getConnection(), w_id);
        application->printGPUInfo();

    } catch (vtrs::RuntimeError& error) {
        vtrs::Logger::fatal(error.what(), "Kind:", error.getKind(), ", Code:", error.getCode());
        return EXIT_FAILURE;
    }

    while (true) {
        auto event = window->pollEvents();

        if (event.kind == vtest::WSIWindowEvent::KEY_PRESS && event.eventDetail == 24) {
            break;
        }
    }

    delete application;
    delete window;

    return EXIT_SUCCESS;
}

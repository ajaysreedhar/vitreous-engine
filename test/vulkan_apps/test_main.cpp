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
#include "engine/except/error.hpp"
#include "engine/platform/logger.hpp"
#include "xcb_client/xcb_window.hpp"
#include "viking_room.hpp"

int main() {
    vtrs::Logger::info("Test: Vulkan Demo VikingRoom");

    vtest::XCBWindow* xcb_window;
    vtest::VikingRoom* application;
    uint32_t window_id;

    try {
        xcb_window = new vtest::XCBWindow();
        window_id = xcb_window->createWindow(800, 600);

        application = vtest::VikingRoom::factory(xcb_window->getConnection(), window_id);
        application->printGPUInfo();
        application->printGPUExtensions();

    } catch (vtrs::RuntimeError& error) {
        vtrs::Logger::fatal(error.what(), "Kind:", error.getKind(), ", Code:", error.getCode());

        delete xcb_window;
        return EXIT_FAILURE;
    }

    vtest::VikingRoom::printIExtensions();

    while (true) {
        auto event = xcb_window->pollEvents();

        if (event.kind == vtest::WSIWindowEvent::KEY_PRESS && event.eventDetail == 24) {
            break;
        }
    }

    delete application;
    delete xcb_window;

    return EXIT_SUCCESS;
}

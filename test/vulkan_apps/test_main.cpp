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
#include <cstring>
#include "platform/standard.hpp"
#include "platform/logger.hpp"
#include "platform/except.hpp"
#include "platform/linux/xcb_client.hpp"
#include "viking_room.hpp"

int main(int argc, char** argv) {

bool is_verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);

#if !defined(VTRS_OS_TYPE_LINUX) && !defined(VTRS_OS_TYPE_WINDOWS)
    vtrs::Logger::fatal("Test runs only on Windows and Linux.");
    return EXIT_FAILURE;
#endif

    vtrs::Logger::info("Test: Vulkan Demo VikingRoom");

    vtrs::XCBClient* xcb_client;
    vtest::VikingRoom* application;
    uint32_t window_id;

    try {
        xcb_client = new vtrs::XCBClient;
        window_id = xcb_client->createWindow(800, 600);

        application = vtest::VikingRoom::factory(xcb_client->getConnection(), window_id);
        application->printGPUInfo();

        if (is_verbose) application->printGPUExtensions();

    } catch (vtrs::PlatformError& error) {
        vtrs::Logger::fatal(error.what(), "Kind:", error.getKind(), ", Code:", error.getCode());

        delete xcb_client;
        return EXIT_FAILURE;
    }

    if (is_verbose) vtest::VikingRoom::printIExtensions();

    while (true) {
        auto event = xcb_client->pollEvents();
        application->drawFrame();

        if (event.kind == vtrs::WSIWindowEvent::KEY_PRESS && event.eventDetail == 24) {
            break;
        }
    }

    delete application;
    delete xcb_client;

    return EXIT_SUCCESS;
}

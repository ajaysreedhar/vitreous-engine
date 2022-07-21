/**
 * test-main.cpp - Vitreous Engine [test-xcb-client]
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

#include "engine/platform/logger.hpp"
#include "xcb-window.hpp"

int main() {
    vtrs::Logger::info("Test: XCB Client");

    auto window = new vtest::XCBWindow();
    window->createWindow(800, 600);

    while (true) {
        auto event = window->pollEvents();

        if (event.kind == vtest::WSIWindowEvent::NO_EVENT) {
            continue;
        }

        if (event.kind == vtest::WSIWindowEvent::KEY_PRESS) {
            if (event.eventDetail == 24) break;
            else if (event.eventDetail == 57) window->createWindow(450, 300);
        }
    }

    delete window;
    return EXIT_SUCCESS;
}
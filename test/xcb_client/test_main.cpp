/**
 * test-main.cpp - Vitreous Engine [test-xcb-client]
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

#include <cstdlib>

#include "platform/logger.hpp"
#include "platform/xcb_client.hpp"

int main() {
    vtrs::Logger::info("Test: XCB Client");

    auto client = new vtrs::XCBClient();
    client->createWindow(800, 600);

    while (true) {
        auto event = client->pollEvents();

        if (event.kind == vtrs::WSIWindowEvent::EMPTY_EVENT) {
            continue;
        }

        if (event.kind == vtrs::WSIWindowEvent::KEY_PRESS) {
            if (event.eventDetail == 24) break;
            else if (event.eventDetail == 57) client->createWindow(450, 300);
        }
    }

    delete client;
    return EXIT_SUCCESS;
}

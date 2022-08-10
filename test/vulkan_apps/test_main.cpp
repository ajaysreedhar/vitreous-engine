/**
 * test_main.cpp - Test cases for Vulkan APIs.
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

#include "platform/except.hpp"
#include "platform/logger.hpp"
#include "platform/linux/xcb_client.hpp"
#include "vulkan_model.hpp"

int testVulkanModel() {
    vtrs::XCBClient* xcb_client;
    vtest::VulkanModel* application;
    uint32_t window_id;

    try {
        xcb_client = new vtrs::XCBClient();
        window_id = xcb_client->createWindow(800, 600);
        application = vtest::VulkanModel::factory(xcb_client, window_id);

    } catch (vtrs::PlatformError& error) {
        vtrs::Logger::fatal(error.what());
        return EXIT_FAILURE;
    }

    application->printGPUInfo();

    while (true) {
        auto event = xcb_client->pollEvents();

        if (event.kind == vtrs::WSIWindowEvent::EMPTY_EVENT) {
            continue;
        }

        if (event.kind == vtrs::WSIWindowEvent::KEY_PRESS && event.eventDetail == 24) {
            vtrs::Logger::info("User pressed Quit [Q] button!");
            break;
        }
    }

    delete xcb_client;
    delete application;

    return EXIT_SUCCESS;
}

int main() {
    vtrs::RendererContext::initialise();

    int status = testVulkanModel();

    vtrs::RendererContext::destroy();
    return status;
}

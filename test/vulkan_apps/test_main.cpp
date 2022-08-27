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

int testVulkanModel(const std::string& model_type, const std::string& texture_file, const std::string& model_file) {
    vtrs::XCBClient* xcb_client;
    vtest::VulkanModel* application;
    vtrs::XCBWindow window;

    try {
        xcb_client = new vtrs::XCBClient();
        window = xcb_client->createWindow(800, 600);
        application = vtest::VulkanModel::factory(xcb_client, window);

    } catch (vtrs::RuntimeError& error) {
        vtrs::Logger::fatal(error.what());
        return EXIT_FAILURE;
    }

    application->printGPUInfo();

    try {
        if (model_type == "object") {
            application->loadModel(texture_file, model_file);

        } else {
            application->loadCube(texture_file);
        }
    } catch (vtrs::RuntimeError& error) {
        vtrs::Logger::fatal(error.what());

        delete application;
        delete xcb_client;

        return EXIT_FAILURE;
    }

    while (true) {
        auto event = xcb_client->pollEvents();

        if (event.kind == vtrs::WSIWindowEvent::WINDOW_EXPOSE) {
            application->rebuildSwapchain();
            continue;
        }

        application->drawFrame();

        if (event.kind == vtrs::WSIWindowEvent::KEY_PRESS && event.eventDetail == 24) {
            vtrs::Logger::info("User pressed Quit [Q] button!");
            break;
        }
    }

    application->waitIdle();

    delete application;
    delete xcb_client;

    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {

    if (argc <= 2) {
        vtrs::Logger::print("Usage: vulkan-test <model-type> <texture-file> [model-file]");
        return 0;
    }

    vtrs::RendererContext::initialise();

    std::string model_type = argv[1];
    std::string texture_file = argv[2];
    std::string model_file = argc >= 4 ? argv[3] : "";

    int status = testVulkanModel(model_type, texture_file, model_file);

    vtrs::RendererContext::destroy();
    return status;
}

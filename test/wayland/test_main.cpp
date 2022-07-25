#include <unistd.h>
#include <thread>
#include "platform/except.hpp"
#include "platform/logger.hpp"
#include "platform/wayland_client.hpp"

void threadRunner(uint32_t* pixels) {
    uint32_t color = 0x0000FF;

    while (color < 0xFFFFFF) {
        for (int y = 0; y < 480; ++y) {
            for (int x = 0; x < 640; ++x) {
                if ((x + y / 8 * 8) % 16 < 8)
                    pixels[y * 480 + x] = color;
                else
                    pixels[y * 480 + x] = 0xFF0000;
            }
        }

        color = color + 0x0000FF;
        sleep(1);
        vtrs::Logger::info("Color", color);
    }
}

int main() {
    vtrs::Logger::info("Test: Wayland Client");
    vtrs::WaylandClient* client;

    try {
        client = vtrs::WaylandClient::factory();
        client->createSurface("Wayland Test");
        vtrs::WaylandClient::displayDispatch();

    } catch (vtrs::PlatformError& error) {
        vtrs::Logger::error(error.getKind(), error.getKind(), error.getCode());
    }

    auto pixels = reinterpret_cast<uint32_t*>(client->getRawPixels());

    if (pixels == nullptr){
        vtrs::Logger::error("No pixel data!");
        return EXIT_FAILURE;
    }

    std::thread t(threadRunner, pixels);

    while (vtrs::WaylandClient::displayDispatch()) {
        vtrs::Logger::debug("Dispatch");
        client->render();
    }

    delete client;

    vtrs::WaylandClient::shutdown();
    return 0;
}
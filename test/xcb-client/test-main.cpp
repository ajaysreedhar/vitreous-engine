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
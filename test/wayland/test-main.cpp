#include <iostream>
#include <wayland-client.h>
#include "engine/platform/logger.hpp"
#include "wl-client.hpp"

int main() {
    TT();

    vtest::WLClient* client = vtest::WLClient::getInstance();
    delete client;

    return 0;
}

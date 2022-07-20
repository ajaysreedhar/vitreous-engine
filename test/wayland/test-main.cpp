#include <iostream>
#include <unistd.h>

#include <wayland-client.h>
#include "engine/platform/logger.hpp"
#include "wl-client.hpp"

void xdg_surface_configure_handler
        (
                void *data,
                struct zxdg_surface_v6 *xdg_surface,
                uint32_t serial
        ) {
    vtrs::Logger::info("Ack surface configure");
    zxdg_surface_v6_ack_configure(xdg_surface, serial);
}

const struct zxdg_surface_v6_listener xdg_surface_listener = {
        .configure = xdg_surface_configure_handler
};

//////////////////////////////////////////////////////////

void xdg_toplevel_configure_handler
        (
                void *data,
                struct zxdg_toplevel_v6 *xdg_toplevel,
                int32_t width,
                int32_t height,
                struct wl_array *states
        ) {
    vtrs::Logger::info("configure toplevel:", width, height);
}

void xdg_toplevel_close_handler
        (
                void *data,
                struct zxdg_toplevel_v6 *xdg_toplevel
        ) {
    vtrs::Logger::info("Close TopLevel");
}

const struct zxdg_toplevel_v6_listener xdg_toplevel_listener = {
        .configure = xdg_toplevel_configure_handler,
        .close = xdg_toplevel_close_handler
};

int main() {
    vtrs::Logger::info("Test: Hello-Wayland");

    vtest::WLClient* client = vtest::WLClient::getInstance();
    zxdg_surface_v6_add_listener(client->getXDGSurface(), &xdg_surface_listener, nullptr);
    zxdg_toplevel_v6_add_listener(client->getXDGTopLevel(), &xdg_toplevel_listener, nullptr);

    client->commitSurface();

    int width = 200;
    int height = 200;
    int stride = width * 4;
    int size = stride * height;  // bytes

    unsigned char* buffer_data = client->getBufferData();

    //*

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {

            struct pixel {
                // little-endian ARGB
                unsigned char blue;
                unsigned char green;
                unsigned char red;
                unsigned char alpha;
            } *px = (struct pixel *) (buffer_data + y * stride + x * 4);

            // draw a cross
            if ((80 < x && x < 120) || (80 < y && y < 120)) {
                // gradient from blue at the top to white at the bottom
                px->alpha = 255;
                px->red = y / height * 255;
                px->green = px->red;
                px->blue = 255;
            } else {
                // transparent
                px->alpha = 0;
            }
        }
    }// */
    client->commitSurface();

    vtrs::Logger::info("Preparing to dispatch");

    for(int loop = 0; loop < 5; loop++) {
        vtrs::Logger::info("Ping", loop);
        vtest::WLClient::dispatch();
        sleep(1);
    }

    delete client;

    vtest::WLClient::destroy();
    return 0;
}

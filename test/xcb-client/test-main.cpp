#include <cstdlib>
#include <xcb/xcb.h>

#include "engine/platform/logger.hpp"

int main() {
    /* Connect to X server */
    xcb_connection_t* connection = xcb_connect(nullptr, nullptr);

    if(connection == nullptr || xcb_connection_has_error(connection)) {
        vtrs::Logger::fatal("Error opening display.");
        return EXIT_FAILURE;
    }

    /* Obtain setup info and access the screen */
    const xcb_setup_t* setup = xcb_get_setup(connection);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;

    if (screen == nullptr) {
        vtrs::Logger::fatal("Error while accessing screen.");
        return EXIT_FAILURE;
    }

    /* Create window */
    xcb_window_t window_id = xcb_generate_id(connection);
    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    uint32_t value_list[2];
    value_list[0] = screen->white_pixel;
    value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_PRESS;

    xcb_create_window(connection, screen->root_depth,
                      window_id, screen->root, 0, 0, 800, 600, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, value_mask, value_list);

    /* Display the window */
    xcb_map_window(connection, window_id);
    xcb_flush(connection);

    /* Execute the event loop */
    uint32_t finished = 0;
    xcb_generic_event_t* event;

    while (!finished && (event = xcb_wait_for_event(connection))) {

        switch(event->response_type) {

            case XCB_KEY_PRESS:
                vtrs::Logger::info("Keycode:", ((xcb_key_press_event_t*)event)->detail);
                finished = 1;
                break;

            case XCB_BUTTON_PRESS:
                vtrs::Logger::info("Button pressed:", ((xcb_button_press_event_t*)event)->detail);
                break;

            case XCB_EXPOSE:
                break;
        }

        free(event);
    }

    /* Disconnect from X server */
    xcb_disconnect(connection);
    return 0;
}
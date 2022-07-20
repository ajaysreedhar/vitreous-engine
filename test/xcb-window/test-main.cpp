#include <cstdlib>
#include <xcb/xcb.h>

#include "engine/platform/logger.hpp"

int main() {
    xcb_connection_t    *conn;
    const xcb_setup_t   *setup;
    xcb_screen_t        *screen;
    xcb_window_t        window_id;
    uint32_t            value_mask;
    uint32_t            value_list[2];
    xcb_generic_event_t *event;
    uint32_t            finished = 0;

    /* Connect to X server */
    conn = xcb_connect(nullptr, nullptr);

    if(conn== nullptr || xcb_connection_has_error(conn)) {
        vtrs::Logger::fatal("Error opening display.");
        return EXIT_FAILURE;
    }

    /* Obtain setup info and access the screen */
    setup = xcb_get_setup(conn);
    screen = xcb_setup_roots_iterator(setup).data;

    if (screen == nullptr) {
        vtrs::Logger::fatal("Error while accessing screen.");
        return EXIT_FAILURE;
    }

    /* Create window */
    window_id = xcb_generate_id(conn);
    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = screen->white_pixel;
    value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_PRESS;
    xcb_create_window(conn, screen->root_depth,
                      window_id, screen->root, 0, 0, 100, 100, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, value_mask, value_list);

    /* Display the window */
    xcb_map_window(conn, window_id);
    xcb_flush(conn);

    /* Execute the event loop */
    while (!finished && (event = xcb_wait_for_event(conn))) {

        switch(event->response_type) {

            case XCB_KEY_PRESS:
                vtrs::Logger::info("Keycode:", ((xcb_key_press_event_t*)event)->detail);
                finished = 1;
                break;

            case XCB_BUTTON_PRESS:
                vtrs::Logger::info("Button pressed:", ((xcb_button_press_event_t*)event)->detail);
                vtrs::Logger::info("X-coordinate:", ((xcb_button_press_event_t*)event)->event_x);
                vtrs::Logger::info("Y-coordinate:", ((xcb_button_press_event_t*)event)->event_y);
                break;

            case XCB_EXPOSE:
                break;
        }
        free(event);
    }

    /* Disconnect from X server */
    xcb_disconnect(conn);
    return 0;
}
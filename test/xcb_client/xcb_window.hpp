/**
 * xcb-window.hpp - Vitreous Engine [test-xcb-client]
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

#pragma once

#include <list>
#include <xcb/xcb.h>

namespace vtest {

typedef struct xcb_connection_t XCBConnection;

struct wsi_window_event {
    enum EventKind: unsigned int {
        KEY_PRESS = 10,
        BUTTON_PRESS,
        CLOSE_BUTTON_PRESS,
        EMPTY_EVENT
    };

    unsigned int kind = EMPTY_EVENT;

    uint32_t eventWindow = -468;
    uint32_t eventDetail = -468;
};

typedef struct wsi_window_event WSIWindowEvent;

class XCBWindow {

private:
    /**
     * @brief Packs window event details from XCB key press event.
     * @param xcb_event Instance of XCB key press event.
     * @return window event details.
     */
    static WSIWindowEvent wsiWindowEvent_(xcb_key_press_event_t*);

    /**
     * @brief Packs window event details from XCB button press event.
     * @param xcb_event Instance of XCB key press event.
     * @return window event details.
     */
    static WSIWindowEvent wsiWindowEvent_(xcb_button_press_event_t*);

    /**
     * @brief Packs window event details from XCB message event.
     * @param xcb_event Instance of XCB key press event.
     * @return window event details.
     */
    static WSIWindowEvent wsiWindowEvent_(xcb_client_message_event_t*);

    xcb_connection_t* m_connection;
    const xcb_setup_t* m_setup;
    xcb_screen_t* m_screen;

    xcb_intern_atom_reply_t* m_protocolReply;
    xcb_intern_atom_reply_t* m_windowReply;
    std::list<uint32_t>* m_WindowIds;

public:
    XCBWindow();
    ~XCBWindow();
    uint32_t createWindow(int, int);
    WSIWindowEvent pollEvents();
    XCBConnection* getConnection();
};

} // namespace vtest
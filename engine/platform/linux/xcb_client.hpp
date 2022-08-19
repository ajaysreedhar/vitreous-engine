/**
 * xcb_client.hpp - Client for X11 with XCB library.
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

#include "platform/standard.hpp"

#ifdef VTRS_OS_TYPE_LINUX

#include <map>
#include <xcb/xcb.h>
#include "platform/ws_interface.hpp"

namespace vtrs {

struct xcb_client_window {
    xcb_window_t identifier = 0;
    unsigned int width = 0;
    unsigned int height = 0;
};

typedef struct xcb_connection_t XCBConnection;
typedef struct xcb_client_window XCBWindow;

class XCBClient {

private: // *** Private members *** //
    xcb_connection_t* m_connection;
    const xcb_setup_t* m_setup;
    xcb_screen_t* m_screen;

    xcb_intern_atom_reply_t* m_protocolReply;
    xcb_intern_atom_reply_t* m_windowReply;
    std::map<xcb_window_t, XCBWindow>* m_windows;

    /**
     * @brief Packs window event details from XCB key press event.
     * @param xcb_event Instance of XCB key press event.
     * @return window event details.
     */
    static vtrs::WSIWindowEvent packWindowEvent_(xcb_key_press_event_t*);

    /**
     * @brief Packs window event details from XCB button press event.
     * @param xcb_event Instance of XCB key press event.
     * @return window event details.
     */
    static WSIWindowEvent packWindowEvent_(xcb_button_press_event_t*);

    /**
     * @brief Packs window event details from XCB message event.
     * @param xcb_event Instance of XCB key press event.
     * @return window event details.
     */
    static WSIWindowEvent packWindowEvent_(xcb_client_message_event_t*);

    static WSIWindowEvent packWindowEvent_(xcb_expose_event_t*);

public: // *** Public members *** //

    /**
     * @brief Initialises the instance.
     * @throw PlatformError If client initialisation fails.
     *
     * Connects to the display server and configures the screen.
     */
    XCBClient();

    /**
     * @brief Cleans up when destroyed.
     * The destructor will iterate the window ids list and closes each window.
     * After closing the window, the xcb_disconnect function is invoked.
     */
    ~XCBClient();

    /**
     * @brief Creates a new window of specified dimensions.
     * @param width Window width in pixels.
     * @param height Window height in pixels.
     * @return Struct containing window information.
     */
    XCBWindow createWindow(unsigned int, unsigned int);

    /**
     * @brief Polls window events.
     * This function does not wait for events. If there are no pending events,
     * a WSIWindowEvent object with empty event kind is returned.
     * @return The WSIWindowEvent object.
     */
    WSIWindowEvent pollEvents();

    /**
     * @brief Returns the XCB connection.
     * @return XCB connection
     */
    XCBConnection* getConnection();
};

} // namespace vtrs

#endif // VTRS_OS_TYPE_LINUX

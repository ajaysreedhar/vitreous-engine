/**
 * xcb_client.cpp - Client for X11 with XCB library.
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

#include <cstdlib>
#include "platform/except.hpp"
#include "platform/logger.hpp"
#include "xcb_client.hpp"

vtrs::WSIWindowEvent vtrs::XCBClient::packWindowEvent_(xcb_key_press_event_t* xcb_event) {
    WSIWindowEvent wsi_event {};
    wsi_event.kind = WSIWindowEvent::KEY_PRESS;
    wsi_event.eventWindow = xcb_event->event;
    wsi_event.eventDetail = xcb_event->detail;

    return wsi_event;
}

vtrs::WSIWindowEvent vtrs::XCBClient::packWindowEvent_(xcb_button_press_event_t* xcb_event) {
    WSIWindowEvent wsi_event {};
    wsi_event.kind = vtrs::WSIWindowEvent::BUTTON_PRESS;
    wsi_event.eventWindow = xcb_event->event;

    return wsi_event;
}

vtrs::WSIWindowEvent vtrs::XCBClient::packWindowEvent_(xcb_client_message_event_t* xcb_event) {
    WSIWindowEvent wsi_event {};

    switch (xcb_event->type) {
        case 256:
            wsi_event.kind = WSIWindowEvent::CLOSE_BUTTON_PRESS;
            break;

        default:
            break;
    }

    wsi_event.eventWindow = xcb_event->window;

    return wsi_event;
}

vtrs::WSIWindowEvent vtrs::XCBClient::packWindowEvent_(xcb_expose_event_t* xcb_event) {
    WSIWindowEvent wsi_event {};
    wsi_event.kind = WSIWindowEvent::WINDOW_EXPOSE;
    wsi_event.eventWindow = xcb_event->window;

    wsi_event.width = xcb_event->width;
    wsi_event.height = xcb_event->height;

    return wsi_event;
}

vtrs::XCBClient::XCBClient() {
    m_windows = new std::map<xcb_window_t, vtrs::XCBWindow>();
    m_connection = xcb_connect(nullptr, nullptr);

    int xcb_error = xcb_connection_has_error(m_connection);

    if(m_connection == nullptr || xcb_error >= 1) {
        throw PlatformError("Unable to initialise XCB connection.", PlatformError::E_TYPE_XCB_CLIENT, xcb_error);
    }

    /* Obtain setup info and access the screen */
    m_setup = xcb_get_setup(m_connection);
    m_screen = xcb_setup_roots_iterator(m_setup).data;

    if (m_screen == nullptr) {
        throw PlatformError("Unable to access XCB screen.", PlatformError::E_TYPE_XCB_CLIENT);
    }

    xcb_intern_atom_cookie_t protocol_cookie = xcb_intern_atom(m_connection, 1, 12,"WM_PROTOCOLS");
    m_protocolReply  = xcb_intern_atom_reply(m_connection, protocol_cookie, nullptr);

    xcb_intern_atom_cookie_t window_cookie = xcb_intern_atom(m_connection, 0, 16,"WM_DELETE_WINDOW");
    m_windowReply  = xcb_intern_atom_reply(m_connection, window_cookie, nullptr);
}

vtrs::XCBClient::~XCBClient() {
    for (auto iterator = m_windows->begin(); iterator != m_windows->end(); iterator++) { // NOLINT(modernize-loop-convert)
        xcb_destroy_window(m_connection, iterator->second.identifier);
    }

    xcb_disconnect(m_connection);

    m_windows->clear();
    delete m_windows;
}

vtrs::XCBWindow vtrs::XCBClient::createWindow(unsigned int width, unsigned int height) {
    vtrs::XCBWindow window {0, width, height};

    window.identifier = xcb_generate_id(m_connection);
    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    uint32_t value_list[2];
    value_list[0] = m_screen->white_pixel;
    value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_PRESS;

    xcb_create_window(m_connection,
                      m_screen->root_depth,
                      window.identifier,
                      m_screen->root, 0, 0,
                      width, height, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      m_screen->root_visual, value_mask, value_list);

    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, window.identifier, m_protocolReply->atom, 4, 32, 1, &m_windowReply->atom);

    xcb_map_window(m_connection, window.identifier);
    xcb_flush(m_connection);

    m_windows->insert(std::pair(window.identifier, window));

    return window;
}

vtrs::WSIWindowEvent vtrs::XCBClient::pollEvents() {
    xcb_generic_event_t* xcb_event = xcb_poll_for_event(m_connection);
    WSIWindowEvent wsi_event;

    if (xcb_event == nullptr) {
        return wsi_event;
    }

    auto response_type = xcb_event->response_type & ~0x80;

    switch (response_type) {
        case XCB_KEY_PRESS:
            wsi_event = packWindowEvent_(reinterpret_cast<xcb_key_press_event_t*>(xcb_event));
            break;

        case XCB_BUTTON_PRESS:
            wsi_event = packWindowEvent_(reinterpret_cast<xcb_button_press_event_t*>(xcb_event));
            break;

        case XCB_CLIENT_MESSAGE:
            wsi_event = packWindowEvent_(reinterpret_cast<xcb_client_message_event_t*>(xcb_event));
            break;

        case XCB_EXPOSE:
            wsi_event = packWindowEvent_(reinterpret_cast<xcb_expose_event_t*>(xcb_event));
            break;

        default:
            vtrs::Logger::debug("Unknown", response_type);
            break;
    }

    free(xcb_event);

    if (wsi_event.kind == WSIWindowEvent::CLOSE_BUTTON_PRESS) {
        xcb_destroy_window(m_connection, wsi_event.eventWindow);
        m_windows->erase(wsi_event.eventWindow);
    }

    return wsi_event;
}

vtrs::XCBConnection *vtrs::XCBClient::getConnection() {
    return m_connection;
}

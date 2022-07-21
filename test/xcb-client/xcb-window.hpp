#pragma once

#include <list>
#include <xcb/xcb.h>

namespace vtest {

struct wsi_window_event {
    enum EventKind: unsigned int {
        KEY_PRESS = 10,
        BUTTON_PRESS,
        CLOSE_BUTTON_PRESS,
        UNKNOWN,
        NO_EVENT
    };

    unsigned int kind = NO_EVENT;

    uint32_t rootWindow = -468;
    uint32_t eventWindow = -468;
    uint32_t childWindow = -468;
    uint32_t eventDetail = -468;
};

typedef struct wsi_window_event WSIWindowEvent;

class XCBWindow {

private:
    static WSIWindowEvent wsiWindowEvent_(xcb_key_press_event_t*);
    static WSIWindowEvent wsiWindowEvent_(xcb_button_press_event_t*);
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
    void createWindow(int, int);
    [[nodiscard]] WSIWindowEvent pollEvents();
};

} // namespace vtest
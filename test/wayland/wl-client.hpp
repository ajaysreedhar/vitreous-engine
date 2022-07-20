#pragma once
#include <wayland-client.h>

namespace vtest {

class WLClient {

private:
    static struct wl_display* m_display;
    WLClient();

public:
    static WLClient* getInstance();

};

} // namespace vtest

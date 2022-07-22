#pragma once

#include <cstring>
#include <wayland-client.h>
#include "xdg_shell.h"

namespace vtest {
typedef struct wl_surface WLSurface;
typedef struct zxdg_surface_v6 XDGSurface;
typedef struct zxdg_toplevel_v6 XDGTopLevel;
typedef struct wl_shm_pool WLSharedPool;
typedef struct wl_buffer WLBuffer;

struct rgba_pixel {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
};

typedef struct rgba_pixel RGBAPixel;

class WLClient {

private:
    static struct wl_display* s_display;
    static struct wl_registry* s_registry;
    static struct wl_shm* s_sharedMem;
    static struct wl_compositor* s_compositor;
    static struct zxdg_shell_v6* s_xdgShell;

    static void s_regHandler(void*, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version) {
        if (0 == strcmp(interface, wl_compositor_interface.name)) {
            vtrs::Logger::info("Bind", wl_compositor_interface.name);

            auto bound = wl_registry_bind(registry, id, &wl_compositor_interface, version);
            s_compositor = reinterpret_cast<struct wl_compositor*>(bound);

        } else if (0 == strcmp(interface, zxdg_shell_v6_interface.name)) {
            vtrs::Logger::info("Bind", zxdg_shell_v6_interface.name);

            auto bound = wl_registry_bind(registry, id, &zxdg_shell_v6_interface, version);
            s_xdgShell = reinterpret_cast<struct zxdg_shell_v6*>(bound);

        } else if (0 == strcmp(interface, wl_shm_interface.name)) {
            vtrs::Logger::info("Bind", wl_shm_interface.name);

            auto bound = wl_registry_bind(registry, id, &wl_shm_interface, version);
            s_sharedMem = reinterpret_cast<struct wl_shm*>(bound);
        }
    }

    static void s_regRemover(void*, struct wl_registry*, uint32_t id) {
        vtrs::Logger::info("Remove registry event: ", id);
    }

    static struct wl_registry_listener s_regListener;

    /* Private class members. */
    int m_sharedFile;
    WLSharedPool* m_pool = nullptr;
    WLBuffer* m_buffer;
    WLSurface* m_wlSurface = nullptr;
    XDGSurface* m_xdgSurface = nullptr;

    WLClient();

public:
    static WLClient* getInstance();
    static void dispatch();
    static void destroy();

    ~WLClient();
    void commitSurface();
    unsigned char* getBufferData();
    XDGSurface* getXDGSurface();
    XDGTopLevel* getXDGTopLevel();

};

} // namespace vtest

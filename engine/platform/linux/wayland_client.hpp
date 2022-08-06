/**
 * wayland_client.hpp - Client that implements Wayland protocol.
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
#define VTRS_MODE_DEBUG 1

#include <wayland-client.h>
#include "third_party/wayland/xdg_shell_protocol.h"

namespace vtrs {

typedef struct wl_display WLDisplay;
typedef struct wl_surface WLSurface;

struct wc_global_state {
    struct wl_shm* sharedmem = nullptr;
    struct wl_compositor* compositor = nullptr;
    struct xdg_wm_base* xdgWmBase = nullptr;
};

struct wc_client_state {
    int surfaceWidth = 640;
    int surfaceHeight = 480;
    long bufferSize = surfaceHeight * surfaceWidth * 4;

    struct wl_surface* wlSurface = nullptr;
    struct xdg_surface* xdgSurface = nullptr;
    struct xdg_toplevel* xdgToplevel = nullptr;
    void* rawPixels = nullptr;
};

class WaylandClient {

private:
    static bool s_isInitialised;

    static struct wl_display* s_display;
    static struct wl_registry* s_registry;
    static struct wc_global_state s_globalState;
    static struct wl_registry_listener s_regListener;
    static struct xdg_surface_listener s_xdgSListener;
    static struct wl_buffer_listener s_bufferListener;

    struct wl_surface* m_surface;

    struct wc_client_state m_clientState;

    static void registryGlobalCb_(void*, struct wl_registry*, uint32_t, const char*, uint32_t);

    static void registryRemoveCb_(void*, struct wl_registry*, uint32_t);

    static void xdgSurfaceConfigCb_(void*, struct xdg_surface*, uint32_t);

    static void bufferReleaseCb_(void*, struct wl_buffer*);

    /**
     * @brief Initialises the global state.
     * @throws vtrs::PlatformError If initialisation fails.
     *
     * This function will attempt to connect to the display,
     * obtain the registry and attach the registry listeners.
     */
    static void initialise_();

    WaylandClient(): m_clientState{}, m_surface(nullptr) {}

public:
    ~WaylandClient();
    static WaylandClient* factory();
    static WLDisplay* getDisplay() {
        return s_display;
    }
    static int displayDispatch();
    static void shutdown();

    void createSurface(const std::string&);
    void render() const;
    [[nodiscard]] void* getRawPixels() const;

    [[nodiscard]] WLSurface* getSurface() {
        return m_surface;
    }
};

} // namespace vtrs
#endif // #ifdef VTRS_OS_TYPE_LINUX
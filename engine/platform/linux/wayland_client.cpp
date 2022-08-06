/**
 * wayland_client.cpp - Client that implements Wayland protocol.
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

#include "platform/logger.hpp"
#include "wayland_client.hpp"

#ifdef VTRS_OS_TYPE_LINUX
#include <unistd.h>
#include <syscall.h>
#include <sys/mman.h>
#include <cstring>
#include "platform/except.hpp"

bool vtrs::WaylandClient::s_isInitialised = false;
struct wl_display* vtrs::WaylandClient::s_display = nullptr;
struct wl_registry* vtrs::WaylandClient::s_registry = nullptr;

struct wl_registry_listener vtrs::WaylandClient::s_regListener {
        vtrs::WaylandClient::registryGlobalCb_,
        vtrs::WaylandClient::registryRemoveCb_
};

struct vtrs::wc_global_state vtrs::WaylandClient::s_globalState {};

struct xdg_surface_listener vtrs::WaylandClient::s_xdgSListener {
    vtrs::WaylandClient::xdgSurfaceConfigCb_
};

struct wl_buffer_listener vtrs::WaylandClient::s_bufferListener {
    vtrs::WaylandClient::bufferReleaseCb_
};

void vtrs::WaylandClient::bufferReleaseCb_(void *, struct wl_buffer* buffer) {
#ifdef VTRS_MODE_DEBUG
    vtrs::Logger::debug("Wayland client: Free buffer.");
#endif
    wl_buffer_destroy(buffer);
}

void vtrs::WaylandClient::registryGlobalCb_(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    auto state = reinterpret_cast<struct wc_global_state*>(data);

#ifdef VTRS_MODE_DEBUG
    vtrs::Logger::debug("Wayland client: Registry event for", interface);
#endif

    if (strcmp(interface, wl_shm_interface.name) == 0) {
        auto bound = wl_registry_bind(registry, name, &wl_shm_interface, version);
        state->sharedmem = reinterpret_cast<struct wl_shm*>(bound);

    } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
        auto bound = wl_registry_bind(registry, name, &wl_compositor_interface, version);
        state->compositor = reinterpret_cast<struct wl_compositor*>(bound);

    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        auto bound = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
        state->xdgWmBase = reinterpret_cast<struct xdg_wm_base*>(bound);
    }
}

void vtrs::WaylandClient::registryRemoveCb_(void*, struct wl_registry*, uint32_t id) {
#ifdef VTRS_MODE_DEBUG
    vtrs::Logger::debug("Wayland client: Registry removed for id:", id);
#endif
}

void vtrs::WaylandClient::xdgSurfaceConfigCb_(void* data, struct xdg_surface* surface, uint32_t serial) {
#ifdef VTRS_MODE_DEBUG
    vtrs::Logger::debug("Wayland client: XDG surface configure event - ", serial);
#endif

    xdg_surface_ack_configure(surface, serial);

    auto state = reinterpret_cast<struct wc_client_state*>(data);

    if (state->rawPixels != nullptr) {
        vtrs::Logger::error("Wayland client: Config discarded!");
        return;
    }

    int stride = state->surfaceWidth * 4;
    size_t size = stride * state->surfaceHeight;

    int shm_fd = static_cast<int>(syscall(SYS_memfd_create, "buffer", 0));
    ftruncate(shm_fd, static_cast<long>(size));

    state->rawPixels = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (state->rawPixels == MAP_FAILED || shm_fd <= -1) {
        vtrs::Logger::error("Wayland client: Memory map failed.");
        close(shm_fd);
        return;
    }

    auto shm_pool = wl_shm_create_pool(s_globalState.sharedmem, shm_fd, static_cast<int>(size));
    auto buffer = wl_shm_pool_create_buffer(shm_pool, 0, state->surfaceWidth, state->surfaceHeight, stride, WL_SHM_FORMAT_XRGB8888);

    wl_buffer_add_listener(buffer, &s_bufferListener, nullptr);

    wl_shm_pool_destroy(shm_pool);
    close(shm_fd);

    wl_surface_attach(state->wlSurface, buffer, 0, 0);
    wl_surface_commit(state->wlSurface);
}

/**
 * @brief Initialises the global state.
 * @throws vtrs::PlatformError If initialisation fails.
 *
 * This function will attempt to connect to the display,
 * obtain the registry and attach the registry listeners.
 */
void vtrs::WaylandClient::initialise_() {
    s_display = wl_display_connect(nullptr);

    if (s_display == nullptr) {
        throw vtrs::PlatformError("Wayland: Unable to connect to display.", vtrs::PlatformError::E_TYPE_WAYLAND_CLIENT, 1);
    }

    s_registry = wl_display_get_registry(s_display);

    if (s_registry == nullptr) {
        throw vtrs::PlatformError("Wayland: Unable to obtain registry.", vtrs::PlatformError::E_TYPE_WAYLAND_CLIENT, 1);
    }

    wl_registry_add_listener(s_registry, &s_regListener, reinterpret_cast<void*>(&s_globalState));
    wl_display_roundtrip(s_display);
}

vtrs::WaylandClient *vtrs::WaylandClient::factory() {
    if (!s_isInitialised) {
        initialise_();
        s_isInitialised = true;
    }

    auto client = new WaylandClient();
    return client;
}

int vtrs::WaylandClient::displayDispatch() {
    return wl_display_dispatch(s_display);
}

void vtrs::WaylandClient::shutdown() {
    if (s_isInitialised) {

#ifdef VTRS_MODE_DEBUG
    vtrs::Logger::debug("Wayland client: Shutting down.");
#endif
        wl_registry_destroy(s_registry);
        wl_display_disconnect(s_display);

#ifdef VTRS_MODE_DEBUG
    vtrs::Logger::debug("Wayland client: Done!");
#endif
    }
}

void vtrs::WaylandClient::createSurface(const std::string& title) {
    m_surface = wl_compositor_create_surface(s_globalState.compositor);

    if (m_surface == nullptr) {
        throw vtrs::PlatformError("Wayland: Unable to create surface.", vtrs::PlatformError::E_TYPE_WAYLAND_CLIENT, 1);
    }

    m_clientState.wlSurface = m_surface;

    m_clientState.xdgSurface = xdg_wm_base_get_xdg_surface(s_globalState.xdgWmBase, m_surface);
    xdg_surface_add_listener(m_clientState.xdgSurface, &s_xdgSListener, &m_clientState);

    m_clientState.xdgToplevel = xdg_surface_get_toplevel(m_clientState.xdgSurface);

    xdg_toplevel_set_title(m_clientState.xdgToplevel, title.c_str());
    wl_surface_commit(m_surface);
}

vtrs::WaylandClient::~WaylandClient() {
#ifdef VTRS_MODE_DEBUG
    vtrs::Logger::debug("Wayland client: Cleaning up...");
#endif

    if (m_clientState.xdgToplevel != nullptr) xdg_toplevel_destroy(m_clientState.xdgToplevel);
    if (m_clientState.xdgSurface != nullptr) xdg_surface_destroy(m_clientState.xdgSurface);
    if (m_clientState.rawPixels != nullptr) munmap(m_clientState.rawPixels, m_clientState.bufferSize);
    wl_surface_destroy(m_surface);

#ifdef VTRS_MODE_DEBUG
    vtrs::Logger::debug("Wayland client: Done!");
#endif
}

void vtrs::WaylandClient::render() const {
    wl_surface_commit(m_clientState.wlSurface);
}

void *vtrs::WaylandClient::getRawPixels() const {
    return m_clientState.rawPixels;
}

#endif // #ifdef VTRS_OS_TYPE_LINUX

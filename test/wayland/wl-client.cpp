#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdexcept>
#include "engine/platform/logger.hpp"
#include "xdg-shell.h"
#include "wl-client.hpp"

struct wl_display* vtest::WLClient::s_display = nullptr;
struct wl_registry* vtest::WLClient::s_registry = nullptr;
struct wl_shm* vtest::WLClient::s_sharedMem = nullptr;
struct wl_compositor* vtest::WLClient::s_compositor = nullptr;
struct zxdg_shell_v6* vtest::WLClient::s_xdgShell = nullptr;

struct wl_registry_listener vtest::WLClient::s_regListener = {
    vtest::WLClient::s_regHandler,
    vtest::WLClient::s_regRemover
};

vtest::WLClient *vtest::WLClient::getInstance() {
    if (s_display != nullptr) {
        return new WLClient();
    }

    s_display = wl_display_connect(nullptr);

    if (s_display == nullptr) {
        throw std::runtime_error("Unable to connect to wl_display.");
    }

    s_registry = wl_display_get_registry(s_display);
    wl_registry_add_listener(s_registry, &s_regListener, nullptr);

    wl_display_roundtrip(s_display);

    return new WLClient();
}

void vtest::WLClient::destroy() {
    if (s_xdgShell != nullptr) {
        vtrs::Logger::info("Destroy zxdg_shell_v6.");
        zxdg_shell_v6_destroy(s_xdgShell);
    }

    if (s_display != nullptr) {
        vtrs::Logger::info("Disconnect wl_display.");
        wl_display_disconnect(s_display);
    }
}

vtest::WLClient::WLClient() : m_buffer(nullptr) {
    if (s_compositor == nullptr) {
        throw std::runtime_error("Instance is not initialised properly.");
    }

    m_sharedFile = -468;
    m_wlSurface  = wl_compositor_create_surface(s_compositor);
    m_xdgSurface = zxdg_shell_v6_get_xdg_surface(s_xdgShell, m_wlSurface);
};

vtest::WLClient::~WLClient() {
    wl_surface_destroy(m_wlSurface);

    if (m_sharedFile >= 0) {
        close(m_sharedFile);
    }

    if (m_pool != nullptr) {
        wl_shm_pool_destroy(m_pool);
    }

    if (m_buffer != nullptr) {
        wl_buffer_destroy(m_buffer);
    }
}

unsigned char *vtest::WLClient::getBufferData() {
    int width = 200;
    int height = 200;
    int stride = width * 4;
    int size = stride * height;

    long fd = syscall(SYS_memfd_create, "buffer", 0);
    m_sharedFile = static_cast<int>(fd) ;

    ftruncate(m_sharedFile, size);

    // turn it into a shared memory pool
    auto raw_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_sharedFile, 0);

    m_pool = wl_shm_create_pool(s_sharedMem, m_sharedFile, size);
    m_buffer = wl_shm_pool_create_buffer(m_pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);

    wl_display_roundtrip(s_display);

    wl_surface_attach(m_wlSurface, m_buffer, 0, 0);
    wl_surface_commit(m_wlSurface);

    return reinterpret_cast<unsigned char*>(raw_data);
}

void vtest::WLClient::dispatch() {
    wl_display_dispatch(s_display);
}

vtest::XDGSurface *vtest::WLClient::getXDGSurface() {
    return m_xdgSurface;
}

vtest::XDGTopLevel* vtest::WLClient::getXDGTopLevel() {
    return zxdg_surface_v6_get_toplevel(m_xdgSurface);
}

void vtest::WLClient::commitSurface() {
    wl_surface_commit(m_wlSurface);
}

set(ENV{VULKAN_SDK} /opt/Vulkan-SDK-1.3.216/x86_64)
find_package(Vulkan REQUIRED FATAL_ERROR)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")

# ---
# Library: vtest-wlclient
# Wayland client library implemented using core wayland-client interface.
# =========================================================================
add_library(vtest-wlclient SHARED
        wayland/xdg-shell.c wayland/xdg-shell.h
        wayland/wl-client.cpp wayland/wl-client.hpp)

target_link_libraries(vtest-wlclient PUBLIC -lwayland-client vtrs-engine)

# ---
# Library: vtest-xcb-client
# XCB Client that provides classes to create XCB windows.
# =========================================================================
add_library(vtest-xcb-client SHARED xcb-client/xcb-window.cpp xcb-client/xcb-window.hpp)
target_link_libraries(vtest-xcb-client PUBLIC -lxcb vtrs-engine)

# ---
# Executable: vulkan-test
# Vulkan application to test graphics capabilities.
# =========================================================================
add_executable(vulkan-test vulkan-apps/viking-room.cpp vulkan-apps/viking-room.hpp vulkan-apps/test-main.cpp)
target_link_libraries(vulkan-test PRIVATE ${Vulkan_LIBRARIES} vtrs-engine vtest-xcb-client)
target_include_directories(vulkan-test PRIVATE ${Vulkan_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

# ---
# Executable: wlclient-test
# Implements a Wayland client using vtest-wlclient library.
# =========================================================================
add_executable(wlclient-test wayland/test-main.cpp)
target_link_libraries(wlclient-test PRIVATE vtest-wlclient)

# ---
# Executable: glfw-test
# Implements a GLFW client on Wayland.
# =========================================================================
add_executable(glfw-test glfw-client/test-main.cpp)
target_link_libraries(glfw-test PRIVATE -lglfw vtrs-engine)

# ---
# Executable: xcb-test
# Implements a XCB client using vtest-xcb-client library.
# =========================================================================
add_executable(xcb-test xcb-client/test-main.cpp)
target_link_libraries(xcb-test PRIVATE -lxcb vtrs-engine vtest-xcb-client)

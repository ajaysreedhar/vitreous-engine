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
        wayland/xdg_shell.c wayland/xdg_shell.h
        wayland/wl_client.cpp wayland/wl_client.hpp)

target_link_libraries(vtest-wlclient PUBLIC -lwayland-client vtrs-engine)

# ---
# Executable: vulkan-test
# Vulkan application to test graphics capabilities.
# =========================================================================
add_executable(vulkan-test vulkan_apps/viking_room.cpp vulkan_apps/viking_room.hpp vulkan_apps/test_main.cpp)
target_link_libraries(vulkan-test PRIVATE ${Vulkan_LIBRARIES} vtrs-engine)
target_include_directories(vulkan-test PRIVATE ${Vulkan_INCLUDE_DIRS})

# ---
# Executable: wlclient-test
# Implements a Wayland client using vtest-wlclient library.
# =========================================================================
add_executable(wlclient-test wayland/test_main.cpp)
target_link_libraries(wlclient-test PRIVATE vtest-wlclient)

# ---
# Executable: glfw-test
# Implements a GLFW client on Wayland.
# =========================================================================
add_executable(glfw-test glfw_client/test_main.cpp)
target_link_libraries(glfw-test PRIVATE -lglfw vtrs-engine)

# ---
# Executable: xcb-test
# Implements a XCB client using engine/platform/xcb_client.
# =========================================================================
add_executable(xcb-test xcb_client/test_main.cpp)
target_link_libraries(xcb-test PRIVATE vtrs-engine)

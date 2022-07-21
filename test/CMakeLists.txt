set(ENV{VULKAN_SDK} /opt/Vulkan-SDK-1.3.216/x86_64)
find_package(Vulkan REQUIRED FATAL_ERROR)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")

# Library: XCB Client
add_library(vtest-xcb-client SHARED xcb-client/xcb-window.cpp xcb-client/xcb-window.hpp)
target_link_libraries(vtest-xcb-client PRIVATE -lxcb vtrs-engine)

# Executable: Viking Room
add_executable(viking-room-test vulkan-apps/viking-room.cpp vulkan-apps/viking-room.hpp vulkan-apps/test-main.cpp)
target_link_libraries(viking-room-test PUBLIC ${Vulkan_LIBRARIES} vtrs-engine)
target_include_directories(viking-room-test PUBLIC ${Vulkan_INCLUDE_DIRS} xcb-client)

# Test: Wayland Client
add_library(vtest-wlclient SHARED
        wayland/xdg-shell.c wayland/xdg-shell.h
        wayland/wl-client.cpp wayland/wl-client.hpp
        )

target_link_libraries(vtest-wlclient PUBLIC -lwayland-client vtrs-engine)

add_executable(wlclient-test wayland/test-main.cpp)
target_link_libraries(wlclient-test PRIVATE vtest-wlclient)

# Test: Wayland GLFW Window
add_executable(glfw-test glfw-client/test-main.cpp)
target_link_libraries(glfw-test PRIVATE -lglfw vtrs-engine)

# Executable: XCB Window
add_executable(xcb-test xcb-client/test-main.cpp)
target_link_libraries(xcb-test PRIVATE -lxcb vtrs-engine vtest-xcb-client)

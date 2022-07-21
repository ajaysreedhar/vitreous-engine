set(ENV{VULKAN_SDK} /opt/Vulkan-SDK-1.3.216/x86_64)
find_package(Vulkan REQUIRED FATAL_ERROR)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")

# Test: Vulkan Demo Application
add_library(vtest-vkdemo SHARED vulkan-demo/application.cpp vulkan-demo/application.hpp)
target_link_libraries(vtest-vkdemo PUBLIC ${Vulkan_LIBRARIES} vtrs-engine)
target_include_directories(vtest-vkdemo PUBLIC ${Vulkan_INCLUDE_DIRS})

add_executable(vkdemo-test vulkan-demo/test-main.cpp)
target_link_libraries(vkdemo-test PUBLIC ${Vulkan_LIBRARIES} vtrs-engine vtest-vkdemo)

# Test: Wayland Client
add_library(vtest-wlclient SHARED
        wayland/xdg-shell.c wayland/xdg-shell.h
        wayland/wl-client.cpp wayland/wl-client.hpp
        )

target_link_libraries(vtest-wlclient PUBLIC -lwayland-client vtrs-engine)

add_executable(wlclient-test wayland/test-main.cpp)
target_link_libraries(wlclient-test PRIVATE vtest-wlclient)

# Test: Wayland GLFW Window
add_executable(glfw-test glfw-window/test-main.cpp)
target_link_libraries(glfw-test PRIVATE -lglfw vtrs-engine)

# Test: XCB Window
add_executable(xcb-test xcb-window/test-main.cpp)
target_link_libraries(xcb-test PRIVATE -lxcb vtrs-engine)

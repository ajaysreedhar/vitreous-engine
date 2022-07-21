set(ENV{VULKAN_SDK} /opt/Vulkan-SDK-1.3.216/x86_64)
find_package(Vulkan REQUIRED FATAL_ERROR)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")

add_library(vtest-wlclient SHARED
        wayland/xdg-shell.c wayland/xdg-shell.h
        wayland/wl-client.cpp wayland/wl-client.hpp
        )

target_link_libraries(vtest-wlclient PUBLIC -lwayland-client vtrs-engine)

# Test: Vulkan Demo Application
add_library(vtest-vkdemo SHARED vulkan-demo/demo-app.cpp vulkan-demo/demo-app.hpp)
target_link_libraries(vtest-vkdemo PUBLIC ${Vulkan_LIBRARIES} vtrs-engine)
target_include_directories(vtest-vkdemo PUBLIC ${Vulkan_INCLUDE_DIRS})

add_executable(vkdemo-test vulkan-demo/test-main.cpp)
target_link_libraries(vkdemo-test PUBLIC ${Vulkan_LIBRARIES} vtrs-engine vtest-vkdemo)

# Test: Wayland Client
add_executable(wlclient-test wayland/test-main.cpp)
target_link_libraries(wlclient-test PRIVATE vtest-wlclient)

# Test: Wayland GLFW
add_executable(wlglfw-test wl-glfw/test-main.cpp)
target_link_libraries(wlglfw-test PRIVATE -lglfw vtrs-engine)

# Test: XCB Window
add_executable(xcb-test xcb-client/test-main.cpp)
target_link_libraries(xcb-test PRIVATE -lxcb vtrs-engine)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")

add_library(vtest-wlclient SHARED
        wayland/xdg-shell.c wayland/xdg-shell.h
        wayland/wl-client.cpp wayland/wl-client.hpp
        )

target_link_libraries(vtest-wlclient PUBLIC -lwayland-client vtrs-engine)

add_executable(wayland-test wayland/test-main.cpp)
target_link_libraries(wayland-test PRIVATE vtest-wlclient)

add_executable(wlglfw-test wl-glfw/test-main.cpp)
target_link_libraries(wlglfw-test PRIVATE -lglfw)

add_executable(xcb-test xcb-window/test-main.cpp)
target_link_libraries(xcb-test PRIVATE -lxcb vtrs-engine)

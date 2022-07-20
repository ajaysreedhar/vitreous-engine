set(BASEPATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/test")

add_library(vtest-wlclient SHARED wayland/wl-client.cpp wayland/wl-client.hpp)
target_link_libraries(vtest-wlclient vtrs-engine -lwayland-client)

add_executable(test-wayland wayland/test-main.cpp)
target_link_libraries(test-wayland PRIVATE vtest-wlclient)
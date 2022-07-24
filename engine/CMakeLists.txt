set(BASEPATH "${CMAKE_SOURCE_DIR}/engine")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/lib")

add_library(vtrs-engine SHARED
        except/runtime.cpp except/runtime.hpp

        platform/standard.hpp
        platform/except.hpp
        platform/logger.cpp             platform/logger.hpp
        platform/ws_interface.hpp
        platform/wayland_client.cpp     platform/wayland_client.hpp
        platform/xcb_client.cpp         platform/xcb_client.hpp
        )

target_link_libraries(vtrs-engine PUBLIC -lxcb -lwayland-client vtrs-tp-wayland)
target_include_directories(vtrs-engine PUBLIC "${CMAKE_SOURCE_DIR}/lib" "${CMAKE_CURRENT_SOURCE_DIR}")

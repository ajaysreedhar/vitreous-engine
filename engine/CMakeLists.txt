set(BASEPATH "${CMAKE_SOURCE_DIR}/engine")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/lib")

add_library(vtrs-platform SHARED
    except/runtime.cpp          except/runtime.hpp
    platform/standard.hpp
    platform/except.hpp
    platform/logger.cpp         platform/logger.hpp
    )
target_include_directories(vtrs-platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(vtrs-linuxpf SHARED
        "${CMAKE_SOURCE_DIR}/lib/third_party/wayland/xdg_shell_protocol.c" "${CMAKE_SOURCE_DIR}/lib/third_party/wayland/xdg_shell_protocol.h"
        platform/linux/wayland_client.cpp platform/linux/wayland_client.hpp
        platform/linux/xcb_client.cpp platform/linux/xcb_client.hpp
        )
target_link_libraries(vtrs-linuxpf PUBLIC -lxcb -lwayland-client)
target_include_directories(vtrs-linuxpf PUBLIC "${CMAKE_SOURCE_DIR}/lib" "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(vtrs-renderer SHARED renderer/instance_factory.cpp renderer/instance_factory.hpp)
target_link_libraries(vtrs-renderer PUBLIC vtrs-platform)
target_include_directories(vtrs-renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

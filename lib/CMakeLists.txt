set(BASEPATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/test")

# ---
# Library: vtrs-tp-wayland
# Wayland helper library build from third party headers.
# =========================================================================
add_library(vtrs-tp-wayland SHARED third_party/wayland/xdg_shell_protocol.c third_party/wayland/xdg_shell_protocol.h)
target_link_libraries(vtrs-tp-wayland PUBLIC -lwayland-client)

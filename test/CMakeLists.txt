find_package(Vulkan REQUIRED FATAL_ERROR)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/test")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/test")

# ---
# Testing engine components
# -
# Executable: triangle-test
# Vitreous engine renderer test.
# =========================================================================
add_executable(renderer-test engine_test/renderer_test.cpp)
target_link_libraries(renderer-test PRIVATE vtrs-platform vtrs-linuxpf vtrs-renderer)
target_include_directories(renderer-test PRIVATE "${CMAKE_SOURCE_DIR}/engine")

# ---
# Executable: xcb-test
# Implements a XCB client using engine/platform/xcb_client.
# =========================================================================
add_executable(xcb-test xcb_client/test_main.cpp)
target_link_libraries(xcb-test PRIVATE vtrs-platform vtrs-linuxpf)

# ---
# Executable: vulkan-test
# Various test cases to test GPU capabilities with Vulkan APIs.
# =========================================================================
add_executable(vulkan-test vulkan_apps/vitreous_model.cpp vulkan_apps/vitreous_model.hpp vulkan_apps/test_main.cpp)
target_include_directories(vulkan-test PRIVATE "${CMAKE_SOURCE_DIR}/engine")

# ---
# Executable: wayland-test
# Implements a Wayland client using engine/platform/xcb_client.
# =========================================================================
add_executable(wayland-test wayland/test_main.cpp)
target_link_libraries(wayland-test PRIVATE vtrs-platform vtrs-linuxpf)
target_include_directories(wayland-test PRIVATE "${CMAKE_SOURCE_DIR}/engine")

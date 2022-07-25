find_package(Vulkan REQUIRED FATAL_ERROR)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/test")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/test")

# ---
# Executable: vulkan-test
# Vulkan application to test graphics capabilities.
# =========================================================================
add_executable(vulkan-test vulkan_apps/viking_room.cpp vulkan_apps/viking_room.hpp vulkan_apps/test_main.cpp)
target_link_libraries(vulkan-test PRIVATE ${Vulkan_LIBRARIES} vtrs-engine)
target_include_directories(vulkan-test PRIVATE ${Vulkan_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/engine")

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

add_executable(wayland-test wayland/test_main.cpp)
target_link_libraries(wayland-test PRIVATE vtrs-engine vtrs-tp-wayland)
target_include_directories(wayland-test PRIVATE "${CMAKE_SOURCE_DIR}/engine")

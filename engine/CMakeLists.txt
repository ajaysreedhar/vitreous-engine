set(BASEPATH "${CMAKE_SOURCE_DIR}/engine")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/lib")

add_library(vtrs-engine SHARED platform/logger.cpp platform/logger.hpp)
target_include_directories(vtrs-engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")


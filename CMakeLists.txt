cmake_minimum_required(VERSION 3.16)

project(Vitreous-Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(lib)
add_subdirectory(engine)
add_subdirectory(test)

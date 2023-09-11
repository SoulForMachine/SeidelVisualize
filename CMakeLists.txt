
cmake_minimum_required(VERSION 3.16)
project(SeidelVisualize)

set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN/")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SeidelVisualize_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SeidelVisualize_BINARY_DIR})

add_subdirectory(Libs/nanogui)
include_directories(Libs)
add_subdirectory(Source)

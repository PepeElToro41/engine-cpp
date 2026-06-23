cmake_minimum_required(VERSION 3.24.0)
project(engine_cpp CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)
add_subdirectory(standalone)
add_subdirectory(vendor)
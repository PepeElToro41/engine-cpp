cmake_minimum_required(VERSION 3.24.0)
project(standalone VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# game/CMakeLists.txt
add_executable(standalone
    src/main.cpp
)


target_link_libraries(standalone PRIVATE
    engine
)

target_compile_features(engine PRIVATE cxx_std_23)
set_target_properties(engine PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

add_custom_command(TARGET standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:engine>
        $<TARGET_FILE_DIR:standalone>
)
cmake_minimum_required(VERSION 3.10.0)
project(engine-cpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodules-ts")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(engine-cpp
    src/main.cpp
    src/ecs/ecs.cpp
    src/ecs/lib/archetype.hpp
    src/ecs/lib/archetype.cpp
    src/ecs/lib/component_record.hpp
    src/ecs/lib/entity_index.hpp
    src/ecs/lib/entity.hpp
    src/ecs/lib/types.hpp
    src/ecs/lib/world.hpp
)

include(CTest)
enable_testing()


include(GenerateExportHeader)
cmake_minimum_required(VERSION 3.24.0)
project(engine VERSION 0.1.0 LANGUAGES C CXX)

set(SDL3_DIR "$ENV{VULKAN_SDK}/cmake")

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(engine SHARED
        src/core/memory/allocation.cpp
        src/core/memory/allocators/arena_allocator.hpp
        src/core/memory/allocators/arena_allocator.cpp
        src/core/memory/allocators/heap_allocator.hpp
        src/core/memory/allocators/heap_allocator.cpp
        src/core/memory/allocators/paged_arena.hpp
        src/core/memory/allocators/paged_stack.hpp
        src/core/memory/allocators/temporal_allocator.hpp 
        src/core/memory/allocation.hpp
        src/core/memory/memory.hpp
        src/core/math/math.cpp
        src/templates/sparse_list.hpp
        src/templates/dynamic_array.hpp
        src/application/application.cpp
        src/application/window.cpp
        src/core/logging.cpp
        src/core/logging.hpp
        src/platform/platformlinux.cpp
        src/platform/platformw64.cpp
        src/render/backends/vulkan.cpp 
        src/utils.cpp 
        src/defines.hpp 
        src/render/backends/vulkan.hpp
        src/render/backend.hpp
        src/export.h
        src/application/application.hpp
        src/application/window.hpp
        src/utils.hpp
        src/helpers.hpp
        src/platform/platform.hpp
        src/ecs/component_record.cpp
        src/ecs/component_record.hpp
        src/ecs/archetype.hpp
        src/ecs/archetype.cpp
        src/ecs/entity_index.cpp
        src/ecs/entity_index.hpp
        src/ecs/ecs_types.hpp
        src/ecs/world.cpp
        src/ecs/world.hpp
        src/ecs/entity.hpp
        src/ecs/entity.cpp
        src/ecs/ecs.hpp
        src/templates/hashmap.hpp
        src/templates/hybrid_hashmap.hpp
        src/ecs/utils/bloom_filter.hpp
)

generate_export_header(engine
    EXPORT_MACRO_NAME ENGINE_API
    EXPORT_FILE_NAME  src/engine/export.h
)

set_target_properties(engine PROPERTIES
    CXX_VISIBILITY_PRESET     hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(engine PRIVATE cxx_std_23)
set_target_properties(engine PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

target_link_libraries(engine PRIVATE
    Vulkan::Vulkan
    Vulkan::volk
    Vulkan::shaderc_combined
    SDL3::SDL3
    imgui
)

function(engine_copy_shaders target_name)
    message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/engine/shaders
            $<TARGET_FILE_DIR:${target_name}>/shaders
            COMMENT "Copying engine shaders to ${CMAKE_SOURCE_DIR}'s output directory"
    )
endfunction()
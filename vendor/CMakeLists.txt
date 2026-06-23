add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_vulkan.cpp
)

set(SDL3_DIR "$ENV{VULKAN_SDK}/cmake")

target_include_directories(imgui PUBLIC
    imgui
    imgui/backends
)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)

target_link_libraries(imgui PUBLIC 
    Vulkan::Vulkan
    SDL3::SDL3
)
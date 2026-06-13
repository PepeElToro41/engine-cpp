#pragma once

#include "export.h"
#include "defines.hpp"

struct SDL_Window;

struct DisplayWindow {
    SDL_Window* sdl_window = nullptr;
    const char* title = nullptr;
    u32 width = 0;
    u32 height = 0;

    DisplayWindow(const char* title, int width, int height);

    bool initialize();
    bool is_valid();
    void destroy();
};


namespace DISPLAY_WINDOW {
    ENGINE_API bool sdl_initialize();
    ENGINE_API void sdl_shutdown();
}

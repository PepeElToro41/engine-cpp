#include <SDL3/SDL.h>
#include "window.hpp"
#include "core/memory/allocators/heap_allocator.hpp"
#include "core/memory/memory.hpp"

DisplayWindow::DisplayWindow(const char* title, int width, int height) {
    this->width = width;
    this->height = height;
    this->title = title;
}

bool DisplayWindow::initialize() {
    SDL_Window* window = SDL_CreateWindow(this->title, this->width, this->height, 0);
    this->sdl_window = window;
    return this->is_valid();
}

bool DisplayWindow::is_valid() {
    return this->sdl_window != nullptr;
}

void DisplayWindow::destroy() {
    if (this->is_valid()) {
        SDL_DestroyWindow(this->sdl_window);
        this->sdl_window = nullptr;
    }
}


bool DISPLAY_WINDOW::sdl_initialize() {
    return SDL_Init(SDL_INIT_VIDEO);
}

void DISPLAY_WINDOW::sdl_shutdown() {
    return SDL_Quit();
}

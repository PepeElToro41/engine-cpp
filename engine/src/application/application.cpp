#include <SDL3/SDL.h>

#include "render/backend.hpp"
#include "render/backends/vulkan.hpp"
#include "application/window.hpp"
#include "core/memory/allocators/heap_allocator.hpp"
#include "core/memory/memory.hpp"
#include "core/logging.hpp"

#include "application.hpp"

Application::Application(const char* title) {
    this->title = title;
    this->running = false;
    
}

bool Application::initialize() {
    DisplayWindow* window = MEMORY::alloc<DisplayWindow>(&HEAP_ALLOCATOR);
    VulkanBackend* backend = MEMORY::alloc<VulkanBackend>(&HEAP_ALLOCATOR);
	
    new (window) DisplayWindow(this->title, 640, 480);
    new (backend) VulkanBackend(this->title);

    this->window = window;
    this->backend = backend;
    backend->window = window;
    
    bool success = window->initialize();
    if (!success) {
        LOG_FATAL("Failed to initialize window");
        return false;
    }

    success = backend->initialize();
    if (!success) {
        LOG_FATAL("Failed to initialize backend");
        return false;
    }

    this->running = true;
    return true;
}

void Application::run() {
    SDL_Event event{};

    while (this->running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                this->running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    this->running = false;
                }
            }
        }
        BEGIN_FRAME_RESULT result = this->backend->begin_frame();
        if (result == BEGIN_FRAME_RESULT::SKIP) {
            continue;
        } else if (result == BEGIN_FRAME_RESULT::FAIL) {
            LOG_FATAL("Failed to begin frame");
            return;
        }
        this->backend->render_frame();
        this->backend->end_frame();

        PLATFORM::sleep(16);
    }
    LOG_INFO("Application shutdown");
}


void Application::shutdown() {
    this->backend->shutdown();
    this->window->destroy();

    MEMORY::free(&HEAP_ALLOCATOR, this->window);
    MEMORY::free(&HEAP_ALLOCATOR, this->backend);
    this->running = false;
}   
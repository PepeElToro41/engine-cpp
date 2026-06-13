#pragma once

#include "render/backend.hpp"
#include "window.hpp"
#include "export.h"

struct ENGINE_API Application {
    const char* title = nullptr;
    bool running = false;

    DisplayWindow* window = nullptr;
    RendererBackend* backend = nullptr;

    Application(const char* title);

    bool initialize();
    void run();
    void shutdown();
};
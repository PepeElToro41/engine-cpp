#pragma once

#include "export.h"
#include "application/window.hpp"

const enum BEGIN_FRAME_RESULT {
    SUCCESS = 1,
    SKIP = 2,
    FAIL = 3
};

struct ENGINE_API RendererBackend {
    DisplayWindow* window;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual BEGIN_FRAME_RESULT begin_frame() = 0;
    virtual void render_frame() = 0;
    virtual void end_frame() = 0;
    virtual ~RendererBackend() = 0;
};
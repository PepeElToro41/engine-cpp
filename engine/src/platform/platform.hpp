#pragma once

#include "defines.hpp"
#include "export.h"

struct PlatformState {
    void* internal;
};

namespace PLATFORM {
    bool init(PlatformState* state, char* app_name);
    void terminate(PlatformState* state);

    void console_error(const char* message, u8 level);
    void console_write(const char* message, u8 level);

    ENGINE_API f64 get_clock();
    ENGINE_API void sleep(unsigned long ms);

    void* malloc(size_t size, size_t alignment);
    void* realloc(void* ptr, size_t size, size_t alignment);

    void free(void* ptr);
    
    void memset(void* ptr, u8 value, size_t size);
    void memcpy(void* dst, const void* src, size_t size);
    void memzero(void* ptr, size_t size);
};
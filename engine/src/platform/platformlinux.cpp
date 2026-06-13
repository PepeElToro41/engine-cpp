#include "defines.hpp"
#include "platform/platform.hpp"
#include "core/logging.hpp"

// Linux platform layer.
#if KPLATFORM_LINUX
 
#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>  // sudo apt-get install libx11-dev
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>  // sudo apt-get install libxkbcommon-x11-dev
#include <sys/time.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>  // nanosleep
#else
#include <unistd.h>  // usleep
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


struct InternalState {
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
}

static char* LEVEL_COLORS[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};

void PLATFORM::console_write(const char* message, u8 level) {
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    printf("\033[%sm%s\033[0m", LEVEL_COLORS[level], message);
}
void PLATFORM::console_error(const char* message, u8 level) {
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    printf("\033[%sm%s\033[0m", LEVEL_COLORS[level], message);
}

void PLATFORM::sleep(u64 ms) {
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if (ms >= 1000) {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

f64 PLATFORM::get_clock() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec * 0.000000001;
}

void* PLATFORM::malloc(size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

void* PLATFORM::realloc(void* ptr, size_t size, size_t alignment) {
    return _aligned_realloc(ptr, size, alignment);
}

void PLATFORM::free(void* ptr) {
    _aligned_free(ptr);
}


#endif
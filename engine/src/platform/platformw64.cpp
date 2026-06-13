#include <string>
#include "defines.hpp"
#include "platform/platform.hpp"

#if KPLATFORM_WINDOWS

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>

struct InternalState {
    HINSTANCE h_instance;
    HWND hwnd;
};

static f64 clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

bool PLATFORM::init(PlatformState* state, char* app_name) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);

    return true;
}

void PLATFORM::terminate(PlatformState* state) {
    // TODO: cleanup.
}

// FATAL,ERROR,WARN,INFO,DEBUG,TRACE
static u8 LEVEL_COLORS[6] = {64, 4, 6, 2, 1, 8};

void PLATFORM::console_write(const char* message, const u8 level) {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(console_handle, LEVEL_COLORS[level]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);   
}

void PLATFORM::console_error(const char* message, u8 level) {
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);

    SetConsoleTextAttribute(console_handle, LEVEL_COLORS[level]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_written, 0);
}

void PLATFORM::sleep(DWORD ms) {
    Sleep(ms);
}

f64 PLATFORM::get_clock() {
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64)now_time.QuadPart * clock_frequency;
}

void* PLATFORM::malloc(const usz size, const usz alignment) {
    return _aligned_malloc(size, alignment);
}

void* PLATFORM::realloc(void* ptr, const usz size, const usz alignment) {
    return _aligned_realloc(ptr, size, alignment);
}

void PLATFORM::memset(void* ptr, const u8 value, const usz size) {
    std::memset(ptr, value, size);
}

void PLATFORM::memcpy(void* dst, const void* src, const usz size) {
    std::memcpy(dst, src, size);
}

void PLATFORM::memzero(void* ptr, const usz size) {
    PLATFORM::memset(ptr, 0, size);
}

void PLATFORM::free(void* ptr) {
    _aligned_free(ptr);
}

 

#endif
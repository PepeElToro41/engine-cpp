#pragma once

#include <cstring>

// ReSharper disable CppInconsistentNaming
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8 = char;
using i16 = short;
using i32 = int;
using i64 = long long;

using Flag = u64;

using usz = unsigned long long;
using f32 = float;
using f64 = double;
// ReSharper restore CppInconsistentNaming

using Bitset = u64;

template <typename T>
struct Slice {
    T* data;
    usz len;

    T* operator[](usz index) {
        return &this->data[index];
    }
};

struct StringView {
    const char* string = nullptr;
    usz len = 0;

    explicit StringView(const char* string) {
        const usz len = strlen(string);
        this->string = string;
        this->len = len;
    }
    
    explicit StringView(const char* string, const usz len) {
        this->string = string;
        this->len = len;
    }

    const char* operator[](const usz index) const {
    	if (index > this->len) return nullptr;
        return &this->string[index];
    }
};


#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif


STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define KPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define KPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define KPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define KPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define KPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define KPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define KPLATFORM_IOS 1
#define KPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define KPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif


#ifdef ENGINE_EXPORT
// Exports
#ifdef _MSC_VER
#define LAPI __declspec(dllexport)
#else
#define LAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define LAPI __declspec(dllimport)
#else
#define LAPI
#endif
#endif

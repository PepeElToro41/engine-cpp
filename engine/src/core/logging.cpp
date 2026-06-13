#include <cstdarg>
#include <iostream>

#include "core/logging.hpp"
#include "platform/platform.hpp"

bool initialize_logging() {
    // TODO: create log file.
    return true;
}


void shutdown_logging() {
    // TODO: cleanup logging/write queued entries.
}


void log_output(const LogLevel level, const char* message, ...) {
    // TODO: use temporal allocator to allocate output string.

    const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
    const bool is_error = level < LOG_LEVEL_WARN;

    constexpr i32 msg_length = 32000;
    char out_message[msg_length] = {};

    va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(out_message, msg_length, message, arg_ptr);
    va_end(arg_ptr);

    char out_message2[msg_length];
    sprintf_s(out_message2, "%s%s\n", level_strings[level], out_message);

    if (is_error) {
        PLATFORM::console_error(out_message2, level);
    } else {
        PLATFORM::console_write(out_message2, level);
    }
}

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}
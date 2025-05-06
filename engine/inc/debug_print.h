#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include <stdio.h>
#include <stdarg.h>
#include <global_config.h>

// Debug print function
static inline void debug_print(const char *format, ...) {
    if (global_config.debug_print_enabled) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

#endif // DEBUG_PRINT_H

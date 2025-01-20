#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include <stdio.h>
#include <stdarg.h>

// Declare the global debug mode variable
extern bool debug_print_enabled;

// Debug print function
static inline void debug_print(const char *format, ...) {
    if (debug_print_enabled == true) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

#endif // DEBUG_PRINT_H

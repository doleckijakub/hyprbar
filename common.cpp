#include "common.hpp"

#include <stdio.h>
#include <stdarg.h>

void __logf(const char *level, int r, int g, int b, const char *format, ...) {
    printf("[" ANSI_RGB_F "%s\e[0m] ", ANSI_RGB_ARG(r, g, b), level);
    
    va_list args;
    va_start(args, format);
        vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

const char *format(const char *fmt, ...) {
    static char buffer[1024] = { 0 };
    
    va_list args;
    va_start(args, fmt);
        vsprintf(buffer, fmt, args);
    va_end(args);
    
    return buffer;
}
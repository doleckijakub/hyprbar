#pragma once

#define eprintf(format, ...) fprintf(stderr, format, ##__VA_ARGS__)

#define ANSI_RGB_F "\e[38;2;%d;%d;%dm"
#define ANSI_RGB_ARG(r, g, b) r, g, b

void __logf(const char *level, int r, int g, int b, const char *format, ...);

#define fatalf(format, ...) __logf("FATAL", 127, 0, 0, format, ##__VA_ARGS__)
#define infof(format, ...)  __logf("INFO", 0, 255, 127, format, ##__VA_ARGS__)
#define tracef(format, ...)  __logf("TRACE", 127, 127, 127, format, ##__VA_ARGS__)

#define dief(why, ...) do { \
    fatalf(why, ##__VA_ARGS__); \
    exit(1); \
} while (0)

#define die(why) dief(why)

const char *format(const char *fmt, ...);
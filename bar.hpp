#pragma once

#include <stdint.h>

// Forward declaration
class Client;

class Bar {
public:

    enum class Position {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT,
    };

    struct Config {
        union {
            int width;
            int height;
        };
        Position position;
    };

    class Canvas {
    public: 

        void set_pixel(int x, int y, uint32_t color);

    private:

        Canvas(const Bar *bar);
        ~Canvas();

        const Bar *__bar;

        int __width, __height;

        struct wl_buffer *__buffer;
        int __shm_fd;
        uint32_t *__data;

        friend class Bar;

    };

    Bar(const Config config);

private:

    Config __config;
    Client *__client;
    int __width;
    int __height;
    int __stride;
    int __bufsize;
    bool __configured = false;

    void draw() const;

    friend class Client;

};
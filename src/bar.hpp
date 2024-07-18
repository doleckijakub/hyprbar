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
    ~Bar();

private:

    Config __config;
    Client *__client;
    int __width;
    int __height;
    int __stride;
    int __bufsize;
    bool __configured = false;

    void draw() const;

	struct wl_surface *surface;
	struct zxdg_output_v1 *xdg_output;
    struct zwlr_layer_surface_v1 *layer_surface;

    static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t w, uint32_t h);
    static void layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface);
    static const struct zwlr_layer_surface_v1_listener layer_surface_listener;

    void show();

    friend class Client;

};
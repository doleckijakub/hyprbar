#pragma once

// C++ includes
#include <vector>
#include <memory>

// Libraries
#include <wayland-client.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// Wayland protocols
#include <xdg-shell.h>
#include <xdg-output-unstable-v1.h>
#include <wlr-layer-shell-unstable-v1.h>

#include "bar.hpp"

class Client {
public:

    static void add_bar(std::shared_ptr<Bar> bar);
    static void start();

private:

    Client();
    ~Client();

    static Client &get_instance();

    std::vector<std::shared_ptr<Bar>> __bars;

    int sock_fd;
    const char *xdgruntimedir;
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct zxdg_output_manager_v1 *output_manager;
    struct wl_output *output;

    // Wayland

    static int allocate_shm_file(size_t size);
    
    static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer);
    static const struct wl_buffer_listener wl_buffer_listener;
    
    static void handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    static void handle_global_remove(void *data, struct wl_registry *registry, uint32_t name);
    static const struct wl_registry_listener registry_listener;

    void init_wayland_connection();

    static struct zwlr_layer_surface_v1 *create_layer_surface(Bar *bar);

    // Font

    FT_Library library;
    FT_Face face;

    void init_font();

    friend class Bar;

};
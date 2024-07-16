// C includes
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// C++ includes

// Libraries
#include <wayland-client.h>

// Wayland protocols
#include <xdg-shell.h>
#include <xdg-output-unstable-v1.h>
#include <wlr-layer-shell-unstable-v1.h>

#define eprintf(format, ...) fprintf(stderr, format, ##__VA_ARGS__)

#define ANSI_RGB_F "\e[38;2;%d;%d;%dm"
#define ANSI_RGB_ARG(r, g, b) r, g, b

void __logf(const char *level, int r, int g, int b, const char *format, ...) {
    printf("[" ANSI_RGB_F "%s\e[0m] ", ANSI_RGB_ARG(r, g, b), level);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

#define tracef(format, ...) __logf("TRACE", 0, 63, 63, format, ##__VA_ARGS__)
#define errorf(format, ...) __logf("ERROR", 255, 0, 0, format, ##__VA_ARGS__)

void die(const char *why) {
    errorf(why);
    exit(1);
}

static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct zwlr_layer_shell_v1 *layer_shell;
static struct zxdg_output_manager_v1 *output_manager;

static void handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    (void) data;
    (void) version;

	if (!strcmp(interface, wl_compositor_interface.name)) {
		compositor = (struct wl_compositor *) wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	} else if (!strcmp(interface, wl_shm_interface.name)) {
		shm = (struct wl_shm *) wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (!strcmp(interface, zwlr_layer_shell_v1_interface.name)) {
		layer_shell = (struct zwlr_layer_shell_v1 *) wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
	} else if (!strcmp(interface, zxdg_output_manager_v1_interface.name)) {
		output_manager = (struct zxdg_output_manager_v1 *) wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 2);
	}
}

static void handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    (void) data;
    (void) registry;
    (void) name;
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove
};

int main() {
	if (!(display = wl_display_connect(NULL))) die("Failed to create a Wayland display");

    struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);
	if (!compositor || !shm || !layer_shell || !output_manager) {
		eprintf("Compositor does not support all needed protocols\n");
		eprintf("These work:\n");
        #define Y X(compositor); \
            X(shm); \
            X(layer_shell); \
            X(output_manager);
        #define X(var) do { if (var) eprintf("\t" ANSI_RGB_F #var "\n", ANSI_RGB_ARG(0, 255, 0)); } while (false)
        Y;
        eprintf("\e[0m");
        #undef X
        eprintf("These don't:\n");
        #define X(var) do { if (!var) eprintf("\t" ANSI_RGB_F #var "\n", ANSI_RGB_ARG(255, 0, 0)); } while (false)
        Y;
        eprintf("\e[0m");
        #undef X
        #undef Y
    }

    printf("All ok... for now.\n");
}
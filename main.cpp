// C includes
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// C++ includes

// Libraries
#include <wayland-client.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// Wayland protocols
#include <xdg-shell.h>
#include <xdg-output-unstable-v1.h>
#include <wlr-layer-shell-unstable-v1.h>

#include "config.hpp"

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

#define dief(why, ...) do { \
    errorf(why, ##__VA_ARGS__); \
    exit(1); \
} while (0)

#define die(why) dief(why)

// Wayland
static const char *xdgruntimedir;
static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct zwlr_layer_shell_v1 *layer_shell;
static struct zxdg_output_manager_v1 *output_manager;
static struct zxdg_output_v1 *xdg_output;
static struct wl_surface *surface;
static struct zwlr_layer_surface_v1 *layer_surface;
static struct wl_output *output;

// FreeType
static FT_Library library;
static FT_Face face;

// Bar
static int width;
static int height;
static int stride;
static int bufsize;
static bool configured;
static void draw_bar();

//
// Wayland stuff
//

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
	} else if (!strcmp(interface, wl_output_interface.name)) {
        output = (wl_output *) wl_registry_bind(registry, name, &wl_output_interface, 1);
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

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t w, uint32_t h) {
	(void) data;
    
    zwlr_layer_surface_v1_ack_configure(surface, serial);
	
    if (configured) return;

	width = w;
	height = h;
	stride = width * 4;
	bufsize = stride * height;
	configured = true;

	draw_bar();
}

static void layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface) {
    (void) data;
    (void) surface;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

static void init_wayland_connection() {
    if (!(xdgruntimedir = getenv("XDG_RUNTIME_DIR")))
		die("Could not retrieve XDG_RUNTIME_DIR");

    if (!(display = wl_display_connect(NULL))) die("Failed to create a Wayland display");

    struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);
	#define Y \
        X(compositor) \
        X(shm) \
        X(layer_shell) \
        X(output_manager) \
        X(output)
    #define X(var) var && 
    if (!(Y true)) {
    #undef X
		// eprintf("Compositor does not support all needed protocols\n");
		// eprintf("These work:\n");
        // #define X(var) if (var) eprintf("\t" ANSI_RGB_F #var "\n", ANSI_RGB_ARG(0, 255, 0));
        // Y
        // eprintf("\e[0m");
        // #undef X
        // eprintf("These don't:\n");
        // #define X(var) if (!var) eprintf("\t" ANSI_RGB_F #var "\n", ANSI_RGB_ARG(255, 0, 0));
        // Y
        // eprintf("\e[0m");
        // #undef X
        // #undef Y
        // exit(1);

        die("Compositor does not support all required wayland protocols");
    }

    if (!(surface = wl_compositor_create_surface(compositor))) {
        die("Failed to create a wayland surface");
    }

    if (!(layer_surface = zwlr_layer_shell_v1_get_layer_surface(layer_shell, surface, output, ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM, "hyprbar"))) {
        die("Failed to create a wlr layer surface");
    }

    zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener, nullptr);

	zwlr_layer_surface_v1_set_size(layer_surface, 0, height);
	zwlr_layer_surface_v1_set_anchor(layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | (bottom ? ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM : ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP));
	zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, height);
	wl_surface_commit(surface);
}

//
// Font stuff
//

static const char* FT_Err_To_String(FT_Error err) {
    #undef FTERRORS_H_
    #define FT_ERRORDEF( e, v, s )  case e: return s;
    #define FT_ERROR_START_LIST     switch (err) {
    #define FT_ERROR_END_LIST       }
    #include FT_ERRORS_H
    return "(Unknown)";
}

static void init_font() {
    if (FT_Error error = FT_Init_FreeType(&library)) dief("Failed to initialize freetype, error code: %d", error);

    switch (FT_Error error = FT_New_Face(library, fontstr, 0, &face)) {
        case 0:
            break;
        case FT_Err_Unknown_File_Format:
            die("Unknown font file format");
            break;
        default:
            dief("Failed to load font, error: %s", FT_Err_To_String(error));
            break;
    }

    if (FT_Error error = FT_Set_Char_Size(face, 48 * 64, 0, 100, 0)) {
        dief("Failed set font character size, error: %s", FT_Err_To_String(error));
    }

    // TODO: bake fonts

    // if (char c = 'S'; FT_Error error = FT_Load_Char(face, c, FT_LOAD_RENDER)) {
    //     dief("Failed load character '%c', error: %s", c, FT_Err_To_String(error));
    // }

    // FT_Bitmap bitmap = face->glyph->bitmap;

    // for (unsigned int i = 0; i < bitmap.rows; ++i) {
    //     for (unsigned int j = 0; j < bitmap.width; ++j) {
    //         if (char ch = (bitmap.buffer[i * bitmap.pitch + j] ? '*' : ' ')) printf("%c%c", ch, ch);
    //     }
    //     printf("\n");
    // }

    FT_Done_Face(face);
}

//
// Bar
//

static void setup_bar() {
    if (!(xdg_output = zxdg_output_manager_v1_get_xdg_output(output_manager, output))) {
        die("Failed to get xdg output");
    }
}

static void draw_bar() {

}

static void clean_up() {
	zwlr_layer_surface_v1_destroy(layer_surface);
	wl_surface_destroy(surface);
	zxdg_output_v1_destroy(xdg_output);
	wl_output_destroy(output);

    FT_Done_FreeType(library);
}

int main() {
	init_wayland_connection();
    init_font();

    atexit(clean_up);

    setup_bar();

    printf("All ok... for now.\n");
    
}
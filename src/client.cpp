#include "client.hpp"

#include "common.hpp"

// C includes
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

Client &Client::get_instance() {
    static Client client;
    return client;
}

Client::Client() {
    init_wayland_connection();
}

Client::~Client() {
    wl_output_destroy(output);

	FT_Done_FreeType(library);
}

void Client::add_bar(const Bar::Config bar_config) {
    auto bar = std::make_shared<Bar>(bar_config);
    get_instance().__bars.push_back(bar);

    if (!(bar->surface = wl_compositor_create_surface(get_instance().compositor))) {
        die("Failed to create a wayland surface");
    }

    if (!(bar->layer_surface = create_layer_surface(bar.get()))) {
        die("Failed to create a wlr layer surface");
    }

    switch (bar->__config.position) {
        case Bar::Position::TOP: {
            zwlr_layer_surface_v1_set_size(bar->layer_surface, 0, bar->__height);
            zwlr_layer_surface_v1_set_anchor(bar->layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);
            break;
        }
        case Bar::Position::BOTTOM: {
            zwlr_layer_surface_v1_set_size(bar->layer_surface, 0, bar->__height);
            zwlr_layer_surface_v1_set_anchor(bar->layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
            break;
        }
        case Bar::Position::LEFT: {
            zwlr_layer_surface_v1_set_size(bar->layer_surface, bar->__width, 0);
            zwlr_layer_surface_v1_set_anchor(bar->layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
            break;
        }
        case Bar::Position::RIGHT: {
            zwlr_layer_surface_v1_set_size(bar->layer_surface, bar->__width, 0);
            zwlr_layer_surface_v1_set_anchor(bar->layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
            break;
        }
    }
    
	zwlr_layer_surface_v1_set_exclusive_zone(bar->layer_surface, bar->__height);
	wl_surface_commit(bar->surface);
}

void Client::start() {
    for (const std::shared_ptr<Bar> &bar : get_instance().__bars) {
        bar->show();

        if (!(bar->xdg_output = zxdg_output_manager_v1_get_xdg_output(get_instance().output_manager, get_instance().output))) {
            die("Failed to get xdg output");
        }
    }

	if (int wl_fd = wl_display_get_fd(get_instance().display)) {
        while (true) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(wl_fd, &rfds);
            FD_SET(get_instance().sock_fd, &rfds);
            FD_SET(STDIN_FILENO, &rfds);

            wl_display_flush(get_instance().display);

            if (select(std::max(get_instance().sock_fd, wl_fd) + 1, &rfds, NULL, NULL, NULL) == -1) {
                if (errno == EINTR)
                    continue;
                else
                    dief("select: %h");
            }
            
            if (FD_ISSET(wl_fd, &rfds))
                if (wl_display_dispatch(get_instance().display) == -1)
                    break;
            
            for (const std::shared_ptr<Bar> &bar : get_instance().__bars) {
                bar->draw();
            }
        }
    }
}

int Client::allocate_shm_file(size_t size) {
	int fd = memfd_create("surface", MFD_CLOEXEC);
	if (fd == -1) die("Failed to create shared memory");
	
    int ret;
	do {
		ret = ftruncate(fd, size);
	} while (ret == -1 && errno == EINTR);
    errno = 0;

	if (ret == -1) {
		close(fd);
		die("Failed to truncate shared memory");
	}

	return fd;
}

// wayland buffer listener

void Client::wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
	(void) data;
    wl_buffer_destroy(wl_buffer);
}

const struct wl_buffer_listener Client::wl_buffer_listener = {
	.release = Client::wl_buffer_release,
};

// wayland registry

void Client::handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    (void) version;

    Client &client = *(Client *) data;

	if (!strcmp(interface, wl_compositor_interface.name)) {
		client.compositor = (struct wl_compositor *) wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	} else if (!strcmp(interface, wl_shm_interface.name)) {
		client.shm = (struct wl_shm *) wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (!strcmp(interface, zwlr_layer_shell_v1_interface.name)) {
		client.layer_shell = (struct zwlr_layer_shell_v1 *) wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
	} else if (!strcmp(interface, zxdg_output_manager_v1_interface.name)) {
		client.output_manager = (struct zxdg_output_manager_v1 *) wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 2);
	} else if (!strcmp(interface, wl_output_interface.name)) {
        client.output = (wl_output *) wl_registry_bind(registry, name, &wl_output_interface, 1);
    }
}

void Client::handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    (void) data;
    (void) registry;
    (void) name;
}

const struct wl_registry_listener Client::registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove
};

// init_wayland_connection

void Client::init_wayland_connection() {
    if (!(xdgruntimedir = getenv("XDG_RUNTIME_DIR")))
		die("Could not retrieve XDG_RUNTIME_DIR");

    if (!(display = wl_display_connect(NULL))) die("Failed to create a Wayland display");

    struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, this);
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
		eprintf("Compositor does not support all needed protocols\n");
		eprintf("These work:\n");
        #define X(var) if (var) eprintf("\t" ANSI_RGB_F #var "\n", ANSI_RGB_ARG(0, 255, 0));
        Y
        eprintf("\e[0m");
        #undef X
        eprintf("These don't:\n");
        #define X(var) if (!var) eprintf("\t" ANSI_RGB_F #var "\n", ANSI_RGB_ARG(255, 0, 0));
        Y
        eprintf("\e[0m");
        #undef X
        #undef Y
        exit(1);

        // die("Compositor does not support all required wayland protocols");
    }
}

struct zwlr_layer_surface_v1 *Client::create_layer_surface(Bar *bar) {
    return zwlr_layer_shell_v1_get_layer_surface(
        get_instance().layer_shell,
        bar->surface,
        get_instance().output,
        ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM,
        "hyprbar"
    );
}

// FreeType

static const char* FT_Err_To_String(FT_Error err) {
    #undef FTERRORS_H_
    #define FT_ERRORDEF( e, v, s )  case e: return s;
    #define FT_ERROR_START_LIST     switch (err) {
    #define FT_ERROR_END_LIST       }
    #include FT_ERRORS_H
    return "(Unknown)";
}

void Client::init_font() {
    if (FT_Error error = FT_Init_FreeType(&library)) dief("Failed to initialize freetype, error code: %d", error);

    switch (FT_Error error = FT_New_Face(library, "/usr/share/fonts/google-noto-vf/NotoSansMono[wght].ttf", 0, &face)) {
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
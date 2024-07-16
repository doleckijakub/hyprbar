#include <stdio.h>
#include <stdlib.h>

void die(const char *why) {
    fprintf(stderr, "[ERROR] %s, aborting.\n", why);
    exit(1);
}

#include <wayland-client.h>

static wl_display *display;

int main() {
	if (!(display = wl_display_connect(NULL))) die("Failed to create a Wayland display");
}
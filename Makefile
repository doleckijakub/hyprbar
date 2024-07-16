all: hyprbar

CPPFLAGS += -Wall -Wextra -Wno-unused-parameter -ggdb

hyprbar: CPPFLAGS += $(shell pkg-config --cflags wayland-client)
hyprbar: LDFLAGS  += $(shell pkg-config --libs   wayland-client)

hyprbar: main.cpp
	g++ -o $@ $(CPPFLAGS) $^ $(LDFLAGS)
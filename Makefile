all: hyprbar

INCLUDE_DIRECTORY := include
SRC_DIRECTORY     := src
BUILD_DIRECTORY   := build

CPPFLAGS += -Wall -Wextra -g
CPPFLAGS += -I$(INCLUDE_DIRECTORY)

WAYLAND_PROTOCOLS=$(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)

$(INCLUDE_DIRECTORY):
	mkdir -p $@

$(SRC_DIRECTORY):
	mkdir -p $@

$(BUILD_DIRECTORY):
	mkdir -p $@

$(INCLUDE_DIRECTORY)/xdg-shell.h: | $(INCLUDE_DIRECTORY)
	$(WAYLAND_SCANNER) client-header $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@
$(SRC_DIRECTORY)/xdg-shell.c: | $(SRC_DIRECTORY)
	$(WAYLAND_SCANNER) private-code $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@
$(BUILD_DIRECTORY)/xdg-shell.o: $(SRC_DIRECTORY)/xdg-shell.c | $(BUILD_DIRECTORY)
	cc -c -o $@ $^
PROTOCOLS += xdg-shell

$(INCLUDE_DIRECTORY)/xdg-output-unstable-v1.h: | $(INCLUDE_DIRECTORY)
	$(WAYLAND_SCANNER) client-header $(WAYLAND_PROTOCOLS)/unstable/xdg-output/xdg-output-unstable-v1.xml $@
$(SRC_DIRECTORY)/xdg-output-unstable-v1.c: | $(SRC_DIRECTORY)
	$(WAYLAND_SCANNER) private-code $(WAYLAND_PROTOCOLS)/unstable/xdg-output/xdg-output-unstable-v1.xml $@
$(BUILD_DIRECTORY)/xdg-output-unstable-v1.o: $(SRC_DIRECTORY)/xdg-output-unstable-v1.c | $(BUILD_DIRECTORY)
	cc -c -o $@ $^
PROTOCOLS += xdg-output-unstable-v1

$(INCLUDE_DIRECTORY)/wlr-layer-shell-unstable-v1.h: | $(INCLUDE_DIRECTORY)
	$(WAYLAND_SCANNER) client-header protocols/wlr-layer-shell-unstable-v1.xml $@
	sed -i -e 's/namespace/_namespace/g' $@
$(SRC_DIRECTORY)/wlr-layer-shell-unstable-v1.c: | $(SRC_DIRECTORY)
	$(WAYLAND_SCANNER) private-code protocols/wlr-layer-shell-unstable-v1.xml $@
$(BUILD_DIRECTORY)/wlr-layer-shell-unstable-v1.o: $(SRC_DIRECTORY)/wlr-layer-shell-unstable-v1.c | $(BUILD_DIRECTORY)
	cc -c -o $@ $^
PROTOCOLS += wlr-layer-shell-unstable-v1

PROTOCOLS_HEADERS := $(patsubst %, $(INCLUDE_DIRECTORY)/%.h, $(PROTOCOLS))
PROTOCOLS_SRCS    := $(patsubst %, $(SRC_DIRECTORY)/%.c, $(PROTOCOLS))
PROTOCOLS_BINS    := $(patsubst %, $(BUILD_DIRECTORY)/%.o, $(PROTOCOLS))

LIBS += wayland-client
LIBS += freetype2

hyprbar: CPPFLAGS += $(shell pkg-config --cflags $(LIBS))
hyprbar: LDFLAGS  += $(shell pkg-config --libs   $(LIBS))

hyprbar: main.cpp $(PROTOCOLS_BINS) | config.hpp $(PROTOCOLS_HEADERS)
	g++ -o $@ $(CPPFLAGS) $^ $(LDFLAGS)

start: hyprbar
	./hyprbar

clean:
	rm -f $(PROTOCOLS_HEADERS)
	rm -f $(PROTOCOLS_SRCS)
	rm -rf build

.PHONY: start clean
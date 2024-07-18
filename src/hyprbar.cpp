#include "common.hpp"
#include "client.hpp"

#include "hyprbar.hpp"

int main() {
    try {
        infof("Starting hyprbar v" HYPRBAR_VERSION);

        Client::add_bar({
            height: 32,
            position: Bar::Position::TOP
        });

        Client::add_bar({
            width: 64,
            position: Bar::Position::BOTTOM
        });

        Client::start();
    } catch (const std::exception &e) {
        dief("%s", e.what());
    }

    return 0;
}
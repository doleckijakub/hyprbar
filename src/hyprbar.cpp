#include "common.hpp"
#include "client.hpp"

#include "hyprbar.hpp"

int main() {
    try {
        infof("Starting hyprbar v" HYPRBAR_VERSION);

        Client::add_bar(std::make_shared<Bar>(Bar::Config {
            height: 32,
            position: Bar::Position::TOP
        }));

        Client::add_bar(std::make_shared<Bar>(Bar::Config {
            width: 64,
            position: Bar::Position::BOTTOM
        }));

        Client::start();
    } catch (const std::exception &e) {
        dief("%s", e.what());
    }

    return 0;
}
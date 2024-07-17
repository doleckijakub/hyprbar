#include "common.hpp"
#include "client.hpp"

#include "config.hpp"

int main() {
    try {
        infof("Starting hyprbar v" HYPRBAR_VERSION);

        Client::add_bar(std::make_shared<Bar>(Bar::Config {
            height: 32,
            position: Bar::Position::TOP
        }));

        Client::start();
    } catch (const std::exception &e) {
        dief("%s", e.what());
    }

    return 0;
}
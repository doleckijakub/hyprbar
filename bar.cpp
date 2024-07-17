#include "bar.hpp"

#include "common.hpp"
#include "client.hpp"

// C++ includes
#include <stdexcept>

// C includes
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void Bar::Canvas::set_pixel(int x, int y, uint32_t color) {
    if (0 > x || x > __width || 0 > y || y > __height) throw std::out_of_range(format("Point (%d, %d) is outside of canvas bounds (width: %d, height: %d)", x, y, __width, __height));
    __data[x + y * __width] = color;
}

Bar::Canvas::Canvas(const Bar *bar) : __bar(bar), __width(bar->__width), __height(bar->__height) {
    if ((__shm_fd = Client::allocate_shm_file(__bar->__bufsize)) == -1) throw std::runtime_error("allocate_shm_file failed");
    if ((__data = (uint32_t *) mmap(NULL, __bar->__bufsize, PROT_READ | PROT_WRITE, MAP_SHARED, __shm_fd, 0)) == MAP_FAILED) throw std::runtime_error("mmap failed");
    
    struct wl_shm_pool *pool = wl_shm_create_pool(Client::get_instance().shm, __shm_fd, __bar->__bufsize);
    __buffer = wl_shm_pool_create_buffer(pool, 0, __width, __height, 4 * __width, WL_SHM_FORMAT_ARGB8888);
    wl_buffer_add_listener(__buffer, &Client::get_instance().wl_buffer_listener, NULL);
    wl_shm_pool_destroy(pool);
}

Bar::Canvas::~Canvas() {
    munmap(__data, __bar->__bufsize);
    wl_surface_set_buffer_scale(Client::get_instance().surface, 1);
    wl_surface_attach(Client::get_instance().surface, __buffer, 0, 0);
    wl_surface_damage_buffer(Client::get_instance().surface, 0, 0, __width, __height);
    wl_surface_commit(Client::get_instance().surface);

    close(__shm_fd);
}

Bar::Bar(const Config config) : __config(config) {
    switch (config.position) {
        case Bar::Position::TOP:
        case Bar::Position::BOTTOM:
            __height = __config.height;
            break;
        case Bar::Position::LEFT:
        case Bar::Position::RIGHT:
            __width = __config.width;
            break;
    }
}

void Bar::draw() const {
    Bar::Canvas canvas(this);

    // auto rgb = [](int r, int g, int b, int a = 0xFF) -> uint32_t {
    //     return (a << 24) | (r << 16) | (g << 8) | b;
    // };

    for (int y = 0; y < __height; y++) {
        for (int x = 0; x < __width; x++) {
            canvas.set_pixel(x, y, (x + y) < 30 ? 0xFF181818 : 0);
        }
    }
}
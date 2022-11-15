#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-client.h>

#include "helpers.h"

static const unsigned WIDTH = 320;
static const unsigned HEIGHT = 200;
static const unsigned CURSOR_WIDTH = 100;
static const unsigned CURSOR_HEIGHT = 59;
static const int32_t CURSOR_HOT_SPOT_X = 10;
static const int32_t CURSOR_HOT_SPOT_Y = 35;

static bool done = false;

void on_button(uint32_t button)
{
    done = true;
}

int main(void)
{
    struct wl_buffer *buffer;
    struct wl_shm_pool *pool;
    struct wl_shell_surface *surface;
    int image;

    hello_setup_wayland();

    image = open("images.bin", O_RDWR);

    if (image < 0) {
        perror("Error opening surface image");
        return EXIT_FAILURE;
    }

    pool = hello_create_memory_pool(image);
    surface = hello_create_surface();
    buffer = hello_create_buffer(pool, WIDTH, HEIGHT);
    hello_bind_buffer(buffer, surface);
    hello_set_cursor_from_pool(pool, CURSOR_WIDTH,CURSOR_HEIGHT, CURSOR_HOT_SPOT_X, CURSOR_HOT_SPOT_Y);
    hello_set_button_callback(surface, on_button);

    while (!done) {
        if (wl_display_dispatch(display) < 0) {
            perror("Main loop error");
            done = true;
        }
    }

    fprintf(stderr, "Exiting sample wayland client...\n");

    hello_free_cursor();
    hello_free_buffer(buffer);
    hello_free_surface(surface);
    hello_free_memory_pool(pool);
    close(image);
    hello_cleanup_wayland();

    return EXIT_SUCCESS;
}



/*
Ref  :  https://drewdevault.com/2017/06/10/Introduction-to-Wayland.html



Obtain a wl_display and use it to obtain a wl_registry.
    Scan the registry for globals and grab a wl_compositor and a wl_shm_pool.
    Use the wl_compositor interface to create a wl_surface.
    Use the wl_shell interface to describe your surface’s role.
    Use the wl_shm interface to allocate shared memory to store pixels in.
    Draw something into your shared memory buffers.
    Attach your shared memory buffers to the wl_surface.

*/
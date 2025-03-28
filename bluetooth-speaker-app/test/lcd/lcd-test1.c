#include <stdio.h>
#include "hal/draw_stuff.h"
#define OLIVEC_IMPLEMENTATION
#include "hal/image_loader.h"
#include <unistd.h>

int main() 
{
    Olivec_Canvas* tux = image_loader_load("./assets/img/tux.png");
    Olivec_Canvas* red = image_loader_load("./assets/img/test-red.bmp");
    Olivec_Canvas* screen = image_loader_image_create(240, 240);
    olivec_fill(*screen, OLIVEC_RGBA(255, 255, 255, 255));
    olivec_sprite_blend(*screen, 0, 0, tux->width, tux->height, *tux);
    olivec_sprite_blend(*screen, 0, 0, red->width, red->height, *red);
    draw_stuff_init();
    // draw_stuff_update_screen("hello world\nhello world\n");
    draw_stuff_image(screen, 0, 0);
    // olivec_test();
    sleep(10);
    draw_stuff_cleanup();
    image_loader_image_free(&tux);
    image_loader_image_free(&red);
    image_loader_image_free(&screen);
}
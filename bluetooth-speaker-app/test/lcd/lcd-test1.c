#include <stdio.h>
#include "hal/draw_stuff.h"
#include "hal/image_loader.h"
#include "ui/load_image_assets.h"
#include "ui/draw_text.h"
#include <unistd.h>

int main() 
{
    load_image_assets_init();

    Olivec_Canvas* tux = image_loader_load("./assets/img/tux.png");
    Olivec_Canvas* red = image_loader_load("./assets/img/test-red.bmp");
    Olivec_Canvas* screen = image_loader_image_create(240, 240);
    Olivec_Canvas* my_txt = draw_text("Hello World ygj!?");

    olivec_fill(*screen, OLIVEC_RGBA(255, 255, 255, 255));
    olivec_sprite_blend(*screen, 0, 0, tux->width / 2, tux->height / 2, *tux);
    olivec_sprite_blend(*screen, 0, 0, red->width, red->height, *red);
    olivec_sprite_blend(*screen, 0, 100, my_txt->width, my_txt->height, *my_txt);

    draw_stuff_init();
    // draw_stuff_update_screen("hello world\nhello world\n");
    draw_stuff_image(screen);
    // olivec_test();
    sleep(10);
    draw_stuff_cleanup();
    image_loader_image_free(&tux);
    image_loader_image_free(&red);
    image_loader_image_free(&screen);
    image_loader_image_free(&my_txt);

    load_image_assets_cleanup();
}
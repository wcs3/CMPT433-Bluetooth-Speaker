#include <stdio.h>
#include "hal/draw_stuff.h"
#include "hal/image_loader.h"
#include "ui/load_image_assets.h"
#include "ui/draw_ui.h"
#include <unistd.h>

int main() 
{
    load_image_assets_init();

    Olivec_Canvas* tux = image_loader_load("./assets/img/tux.png");
    Olivec_Canvas* red_c = image_loader_load("./assets/img/red_circle.png");
    
    Olivec_Canvas* screen = image_loader_image_create(240, 240);

    olivec_fill(*screen, OLIVEC_RGBA(255, 255, 255, 255));
    olivec_sprite_blend(*screen, 50, 50, tux->width, tux->height, *tux);
    olivec_sprite_blend(*screen, 0, 0, red_c->width / 2, red_c->height / 2, *red_c);

    draw_stuff_init();
    // draw_stuff_update_screen("hello world\nhello world\n");
    draw_stuff_screen(screen);
    // olivec_test();
    sleep(10);
    draw_stuff_cleanup();
    image_loader_image_free(&tux);
    image_loader_image_free(&red_c);
    image_loader_image_free(&screen);


    load_image_assets_cleanup();
}
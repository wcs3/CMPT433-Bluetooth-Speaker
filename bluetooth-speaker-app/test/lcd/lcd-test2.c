#include <stdio.h>
#include "hal/draw_stuff.h"
#include "hal/image_loader.h"
#include "ui/load_image_assets.h"
#include "ui/draw_ui.h"
#include <unistd.h>

int main() 
{
    load_image_assets_init();

    // Olivec_Canvas* screen = draw

    Olivec_Canvas* screen = image_loader_image_create(240, 240);
    Olivec_Canvas* album = draw_ui_text("Album Name");
    Olivec_Canvas* track = draw_ui_text("Track Name");
    Olivec_Canvas* time_txt = draw_ui_text("0:11 / 2:11");
    Olivec_Canvas* time_bar = draw_ui_progress_bar(120, 5, 0.89f, OLIVEC_RGBA(255, 0, 0, 255));
    Olivec_Canvas* volume_bar = draw_ui_progress_bar(120, 5, 0.5f, OLIVEC_RGBA(0, 0, 255, 255));
    Olivec_Canvas* volume_icon = load_image_assets_get_volume_icon();

    olivec_fill(*screen, OLIVEC_RGBA(255, 255, 255, 255));
    draw_ui_blend_centered(*screen, *album, 10);
    draw_ui_blend_centered(*screen, *track, 80);
    draw_ui_blend_centered(*screen, *time_bar, 105);
    draw_ui_blend_centered(*screen, *time_txt, 115);
 
    int vol_bar_x = draw_ui_blend_centered(*screen, *volume_bar, 150);
    olivec_sprite_blend(*screen, vol_bar_x, 160, volume_icon->width, volume_icon->height, *volume_icon);

    draw_stuff_init();
    // draw_stuff_update_screen("hello world\nhello world\n");
    draw_stuff_screen(screen);
    // olivec_test();
    sleep(10);
    draw_stuff_cleanup();


    load_image_assets_cleanup();
}
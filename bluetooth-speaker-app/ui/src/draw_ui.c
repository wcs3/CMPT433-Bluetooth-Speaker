#include "hal/olive.h"
#include "hal/image_loader.h"
#include "ui/load_image_assets.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

Olivec_Canvas* draw_ui_text(char* str)
{
    
    size_t str_len = strlen(str);
    Olivec_Canvas* text_img = image_loader_image_create(LOAD_IMAGE_ASSETS_CHAR_WIDTH * str_len, LOAD_IMAGE_ASSETS_CHAR_HEIGHT);
    olivec_fill(*text_img, OLIVEC_RGBA(0xFF, 0xFF, 0xFF, 0));
    for(size_t i = 0; i < str_len; i++) {
        Olivec_Canvas* char_img = load_image_assets_get_char(str[i]);
        olivec_sprite_blend(*text_img, LOAD_IMAGE_ASSETS_CHAR_WIDTH * i, 0, char_img->width, char_img->height, *char_img);
    }
    return text_img;
}

Olivec_Canvas* draw_ui_progress_bar(int width, int height, float progress, uint32_t primary_colour)
{
    int progress_length = round(progress * width);
    Olivec_Canvas* bar_img = image_loader_image_create(width, height);
    olivec_fill(*bar_img, OLIVEC_RGBA(0xD3, 0xD3, 0xD3, 0xFF));
    olivec_rect(*bar_img, 0, 0, progress_length, height, primary_colour);
    return bar_img;
}

int draw_ui_blend_centered(Olivec_Canvas canvas, Olivec_Canvas sprite, int y)
{
    int x = (canvas.width - sprite.width) / 2;
    olivec_sprite_blend(canvas, x, y, sprite.width, sprite.height, sprite);
    return x;
}
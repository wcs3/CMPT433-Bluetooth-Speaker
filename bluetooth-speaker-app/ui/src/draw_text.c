#include "hal/olive.h"
#include "hal/image_loader.h"
#include "ui/load_image_assets.h"
#include <string.h>
#include <stdio.h>

Olivec_Canvas* draw_text(char* str)
{
    
    size_t str_len = strlen(str);
    Olivec_Canvas* text_img = image_loader_image_create(LOAD_IMAGE_ASSETS_CHAR_WIDTH * str_len, LOAD_IMAGE_ASSETS_CHAR_HEIGHT);
    olivec_fill(*text_img, OLIVEC_RGBA(255, 255, 255, 0));
    for(size_t i = 0; i < str_len; i++) {
        Olivec_Canvas* char_img = load_image_assets_get_char(str[i]);
        olivec_sprite_blend(*text_img, LOAD_IMAGE_ASSETS_CHAR_WIDTH * i, 0, char_img->width, char_img->height, *char_img);
    }
    return text_img;
}
// defines a state machine for a button
#ifndef _LOAD_IMAGE_ASSETS_H
#define _LOAD_IMAGE_ASSETS_H

#define LOAD_IMAGE_ASSETS_CHAR_WIDTH 10
#define LOAD_IMAGE_ASSETS_CHAR_HEIGHT 20

int load_image_assets_init();
// returns NULL if no image for a character exists
Olivec_Canvas* load_image_assets_get_char(char c);
Olivec_Canvas* load_image_assets_get_volume_icon();
void load_image_assets_cleanup();
int draw_ui_blend_centered(Olivec_Canvas canvas, Olivec_Canvas sprite, int y);

#endif
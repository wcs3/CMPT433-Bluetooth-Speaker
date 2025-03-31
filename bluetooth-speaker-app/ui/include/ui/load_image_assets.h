// defines a state machine for a button
#ifndef _LOAD_IMAGE_ASSETS_H
#define _LOAD_IMAGE_ASSETS_H

#define LOAD_IMAGE_ASSETS_CHAR_WIDTH 10
#define LOAD_IMAGE_ASSETS_CHAR_HEIGHT 20

int load_image_assets_init();
// returns NULL if no image for a character exists
Olivec_Canvas* load_image_assets_get_char(char c);
void load_image_assets_cleanup();

#endif
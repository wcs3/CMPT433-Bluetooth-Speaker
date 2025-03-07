// Functions for loading and storing images in memory
#ifndef _APP_IMAGE_LOADER_H
#define _APP_IMAGE_LOADER_H

#include "stdint.h"

typedef struct image_loader_rgba {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} image_loader_rgba;

typedef struct image_loader_image {
    int x;
    int y;
    image_loader_rgba* pixels;
} image_loader_image;

int image_loader_init();

image_loader_image* image_loader_load(const char* path);

void image_loader_cleanup();

#endif
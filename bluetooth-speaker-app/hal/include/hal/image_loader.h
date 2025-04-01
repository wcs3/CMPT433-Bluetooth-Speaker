// Functions for loading and storing images in memory
#ifndef _APP_IMAGE_LOADER_H
#define _APP_IMAGE_LOADER_H

#include "stdint.h"
#include "hal/olive.h"

Olivec_Canvas* image_loader_image_create(int x, int y);

void image_loader_image_free(Olivec_Canvas** image);

Olivec_Canvas* image_loader_load(const char* path);

#endif
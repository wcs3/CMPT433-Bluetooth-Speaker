// Functions for loading and storing images in memory
#ifndef _APP_IMAGE_LOADER_H
#define _APP_IMAGE_LOADER_H

#include "stdint.h"
#include "hal/olive.h"

// create a heap allocated x by y image. 
Olivec_Canvas* image_loader_image_create(int x, int y);

// create a heap allocated image from a file
Olivec_Canvas* image_loader_load(const char* path);

// free an Olivec_Canvas created by image_loader_image_create or image_loader_load
void image_loader_image_free(Olivec_Canvas** image);

#endif
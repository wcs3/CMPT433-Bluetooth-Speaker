#include "hal/image_loader.h"

#define OLIVEC_IMPLEMENTATION
#include "hal/olive.h"

#define STB_IMAGE_IMPLEMENTATION
#include "hal/stb_image.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

Olivec_Canvas* image_loader_image_create(int x, int y)
{
    Olivec_Canvas* new_image = malloc(sizeof(*new_image));
    uint32_t* pixels = malloc(sizeof(*pixels) * x * y);
    *new_image = olivec_canvas(pixels, x, y, x);
    return new_image;
}

void image_loader_image_free(Olivec_Canvas** image)
{
    free((*image)->pixels);
    free(*image);
    *image = NULL;
}

Olivec_Canvas* image_loader_load(const char* path)
{
    int x, y, n_channels;
    stbi_uc* data = stbi_load(path, &x, &y, &n_channels, 0);
    if(data == NULL) {
        fprintf(stderr, "failed to load %s\n", path);
        return NULL;
    }

    if(n_channels < 3) {
        fprintf(stderr, "%s has less than 3 channels \n", path);
        return NULL;
    }

    Olivec_Canvas* new_image = image_loader_image_create(x, y);
    if(n_channels == 3) {
        for(int i = 0; i < x * y; i ++) {
            uint8_t r = data[i * 3];
            uint8_t g = data[i * 3 + 1];
            uint8_t b = data[i * 3 + 2];
            new_image->pixels[i] = OLIVEC_RGBA(r, g, b, 0xFF);
        }
    }
    if(n_channels == 4) {
        for(int i = 0; i < x * y; i ++) {
            uint8_t r = data[i * 4];
            uint8_t g = data[i * 4 + 1];
            uint8_t b = data[i * 4 + 2];
            uint8_t a = data[i * 4 + 3];
            new_image->pixels[i] = OLIVEC_RGBA(r, g, b, a);
        }
    }

    free(data);

    return new_image;
}
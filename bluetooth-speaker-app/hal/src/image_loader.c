#include "hal/image_loader.h"
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "hal/stb_image.h"
#include <stdlib.h>
#include <string.h>

image_loader_image** images = NULL;
int num_images = 0;
const int MAX_IMAGES = 128;

int image_loader_init() 
{
    images = malloc(sizeof(images[0]) * MAX_IMAGES);
    num_images = 0;
    return 0;
}

image_loader_image* image_loader_image_create(int x, int y)
{
    image_loader_image* new_img = malloc(sizeof(image_loader_image));
    new_img->x = x;
    new_img->y = y;
    new_img->pixels = malloc((x * y) * sizeof(image_loader_rgba));
    return new_img;
}

void image_loader_image_free(image_loader_image* image)
{
    free(image->pixels);
    free(image);
}

image_loader_image* image_loader_load(const char* path) 
{
    if(num_images == MAX_IMAGES) {
        fprintf(stderr, "image limit of %d reached\n", MAX_IMAGES);
        return NULL;
    }

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

    image_loader_image* new_image = image_loader_image_create(x, y);
    if(n_channels == 3) {
        for(int i = 0; i < x * y; i ++) {
            new_image->pixels[i].r = data[i * 3];
            new_image->pixels[i].g = data[i * 3 + 1];
            new_image->pixels[i].b = data[i * 3 + 2];
            new_image->pixels[i].a = 0xFF;
        }
    }
    if(n_channels == 4) {
        for(int i = 0; i < x * y; i ++) {
            new_image->pixels[i].r = data[i * 4];
            new_image->pixels[i].g = data[i * 4 + 1];
            new_image->pixels[i].b = data[i * 4 + 2];
            new_image->pixels[i].a = data[i * 4 + 3];
        }
    }

    images[num_images] = new_image;
    num_images++;

    stbi_image_free(data);
    return new_image;
}

void image_loader_cleanup() 
{
    for(int i = 0; i < num_images; i++) {
        image_loader_image_free(images[i]);
    }
    free(images);
    num_images = 0;
    images = NULL;
}

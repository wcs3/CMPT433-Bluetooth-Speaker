#include "hal/image_loader.h"
#include <string.h>
#include <stdio.h>

#define NUM_CHARS 256
#define BUF_SIZE 256

static const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~`!@#$%^&*()[{]}\\|;:\"\',<.>/? ";

static Olivec_Canvas* char_images[NUM_CHARS];
static Olivec_Canvas* volume_icon = NULL;

int load_image_assets_init()
{
    char buf[BUF_SIZE];

    for(int i = 0; i < BUF_SIZE; i++) {
        char_images[i] = NULL;
    }

    for(size_t i = 0; i < sizeof(characters); i++) {
        int c = characters[i];
        snprintf(buf, BUF_SIZE, "./assets/img/characters/char_%d.png", c);
        char_images[c] = image_loader_load(buf);
        if(char_images[c] == NULL) {
            fprintf(stderr, "load_image_assets_init failed to load %s\n", buf);
            return 1;
        }
    }

    volume_icon = image_loader_load("./assets/img/icon/volume_icon.png");
    if(volume_icon == NULL) {
        fprintf(stderr, "load_image_assets_init failed to load %s\n", "./assets/img/icon/volume_icon.png");
        return 2;
    }

    return 0;
}

Olivec_Canvas* load_image_assets_get_char(char c)
{
    if(char_images[(int) c] != NULL) {
        return char_images[(int) c];
    } else {
        return char_images[0];
    }
}

Olivec_Canvas* load_image_assets_get_volume_icon()
{
    return volume_icon;
}

void load_image_assets_cleanup()
{
    for(size_t i = 0; i < strlen(characters); i++) {
        int c = characters[i];
        image_loader_image_free(&char_images[c]);
    }
    image_loader_image_free(&volume_icon);
}
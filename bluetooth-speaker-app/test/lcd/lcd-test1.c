#include <stdio.h>
#include "hal/draw_stuff.h"
#include "hal/image_loader.h"
#include <unistd.h>

int main() 
{
    image_loader_init();
    image_loader_image* img =  image_loader_load("./assets/img/test-red.bmp");
    draw_stuff_init();
    // draw_stuff_update_screen("hello world\nhello world\n");
    draw_stuff_image(img, 0, 0);
    sleep(10);
    draw_stuff_cleanup();
    image_loader_cleanup();
}
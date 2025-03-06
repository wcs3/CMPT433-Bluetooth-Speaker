#include <stdio.h>
#include "hal/draw_stuff.h"
#include <unistd.h>

int main() 
{
    draw_stuff_init();
    // draw_stuff_update_screen("hello world\nhello world\n");
    draw_stuff_update_screen2();
    draw_stuff_cleanup();
}
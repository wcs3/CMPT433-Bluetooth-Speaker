#include <stdio.h>
#include "hal/rotary_encoder.h"
#include <unistd.h>

volatile int pos = 0;
void on_turn(bool clockwise) 
{
    if(clockwise) {
        pos++;
        printf("c  %d\n", pos);
    } else {
        pos--;
        printf("cc %d\n", pos);
    }
}

int main() 
{
    rotary_encoder_init();
    rotary_encoder_set_turn_listener(on_turn);
    sleep(60);
    rotary_encoder_cleanup();
}
// Register listeners for turns and presses of the rotary encoder
#ifndef _ROTARY_ENCODER_H
#define _ROTARY_ENCODER_H

#include <stdbool.h>

// returns 0 if successful
int rotary_encoder_init(void);

// bool argument is true for clockwise, false for counter clockwise
void rotary_encoder_set_turn_listener(void (* on_turn)(bool));

// Set listener to run once when rotary encoder is pressed.
void rotary_encoder_set_press_listener(void (* on_press)());

void rotary_encoder_cleanup(void);

#endif
// Detects turns in rotary encoder
#ifndef _ROTARY_ENCODER_H
#define _ROTARY_ENCODER_H

#include <stdbool.h>

int rotary_encoder_init(void);
// bool argument is true for clockwise, false for counter clockwise
void rotary_encoder_set_turn_listener(void (* on_turn)(bool));
void rotary_encoder_cleanup(void);

#endif
#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

struct joystick_position {
    float x;
    float y;
};

int joystick_init(void);
int joystick_read_x(float* result);
int joystick_read_y(float* result);
int joystick_read_xy(struct joystick_position* result);

#endif
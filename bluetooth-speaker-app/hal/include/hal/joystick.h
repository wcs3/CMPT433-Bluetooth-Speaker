// functions for sampling the joystick and setting listeners for presses, up, and down
#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

struct joystick_position {
    float x;
    float y;
};

// returns 0 if successful
int joystick_init(void);

// from -1.0 to 1.0, right being 1.0. Returns 0 is successful
int joystick_read_x(float* result);

// from -1.0 to 1.0, up being 1.0. Returns 0 is successful
int joystick_read_y(float* result);

// from -1.0 to 1.0, up being 1.0, right being 1.0. Returns 0 is successful
int joystick_read_xy(struct joystick_position* result);

// triggers when the joystick button is pressed
void joystick_set_on_press_listener(void (*on_press)());

// triggers when a joystick is moved up, triggers repeatedly at regular intervals when the joystick is held in the up position
void joystick_set_on_up_listener(void (*on_up)());

// triggers when a joystick is moved down, triggers repeatedly at regular intervals when the joystick is held in the down position
void joystick_set_on_down_listener(void (*on_down)());

void joystick_set_on_left_listener(void (*on_left)());

void joystick_set_on_right_listener(void (*on_right)());

void joystick_cleanup(void);
#endif
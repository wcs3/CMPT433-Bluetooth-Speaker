// defines a state machine for a button
#ifndef _BUTTON_STATE_MACHINE_H_
#define _BUTTON_STATE_MACHINE_H_

#include <stdbool.h>

struct button_state;
struct button_state_event;
struct button_state_machine;

struct button_state_event {
    struct button_state* next_state;
    void (*action)(long, struct button_state_machine*);
};

struct button_state {
    struct button_state_event rising;
    struct button_state_event falling;
};

struct button_state_machine {
    struct button_state states[2];
    struct button_state* curr_state;
    long last_press_time;
};

// create a heap allocated state machine. on_press takes in current time as arg1 and a pointer to its state machine as arg2
struct button_state_machine* button_state_machine_init(void (*on_press)(long, struct button_state_machine*));

// change the state of a state machine
void button_state_machine_update(struct button_state_machine* state_machine, bool is_rising, long curr_time);

// free the memory associated with a state machine
void button_state_machine_free(struct button_state_machine** state_machine);
#endif
#include "hal/button_state_machine.h"
#include <stdlib.h>

struct button_state_machine* button_state_machine_init(void (*on_press)(long, struct button_state_machine*))
{
    struct button_state_machine* state_machine = malloc(sizeof(*state_machine));
    struct button_state rest = {
        .rising = {&state_machine->states[0], NULL},
        .falling = {&state_machine->states[1], on_press}
    };
    struct button_state down = {
        .rising = {&state_machine->states[0], NULL},
        .falling = {&state_machine->states[1], NULL}
    };
    state_machine->states[0] = rest;
    state_machine->states[1] = down;
    state_machine->curr_state = &state_machine->states[0];
    return state_machine;
}

void button_state_machine_update(struct button_state_machine* state_machine, bool is_rising, long curr_time)
{
    struct button_state_event* state_event = NULL;
    if (is_rising) {
        state_event = &state_machine->curr_state->rising;
    } else {
        state_event = &state_machine->curr_state->falling;
    }
    if(state_event->action != NULL) {
        state_event->action(curr_time, state_machine);
    }
    state_machine->curr_state = state_event->next_state;
}


void button_state_machine_free(struct button_state_machine** state_machine)
{
    free(*state_machine);
    *state_machine = NULL;
}
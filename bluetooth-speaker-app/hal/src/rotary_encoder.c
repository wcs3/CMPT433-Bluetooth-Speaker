#include "hal/rotary_encoder.h"
#include "hal/button_state_machine.h"
#include "hal/lg_gpio_samples_func.h"
#include "hal/time_util.h"
#include "lgpio.h"
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <gpiod.h>
#include <pthread.h>

enum GPIO_CHIPS {
    CHIP_0 = 0,
    CHIP_1 = 1,
    CHIP_2 = 2
};


static const int ROTARY_ENCODER_A_GPIO = 7;
static const int ROTARY_ENCODER_B_GPIO = 8;
static const int ROTARY_ENCODER_BUTTON_GPIO = 10;
static const long BUTTON_DEBOUNCE = 50;

static int chip_2_handle = -1;
static int chip_0_handle = -1;
static bool initialized = false;
void do_nothing(void) {}
void do_nothing_b(bool b __attribute__((unused))) {}
void (* _Atomic on_turn_listener)(bool) = do_nothing_b;
void (* _Atomic on_press_listener)() = do_nothing;

static bool going_clockwise = true;

static void transition_clockwise_start()
{
    going_clockwise = true;
}

static void transition_clockwise_end()
{
    if(going_clockwise) {
        on_turn_listener(true);
    }
}

static void transition_counterclockwise_start()
{
    going_clockwise = false;
}

static void transition_counterclockwise_end()
{
    if(!going_clockwise) {
        on_turn_listener(false);
    }
}
// from gpio_statemachine_demo
struct state_event {
    struct state* next_state;
    void (*action)();
};
struct state {
    struct state_event a_rising;
    struct state_event a_falling;
    struct state_event b_rising;
    struct state_event b_falling;
};
struct state states[] = {
    { // Rest (both risen)
        .a_rising = {&states[0], NULL},
        .a_falling = {&states[1], transition_clockwise_start},
        .b_rising = {&states[0], NULL},
        .b_falling = {&states[3], transition_counterclockwise_start}
    },
    { // A fallen B risen
        .a_rising = {&states[0], transition_counterclockwise_end},
        .a_falling = {&states[1], NULL},
        .b_rising = {&states[1], NULL},
        .b_falling = {&states[2], NULL}
    },
    { // A fallen B fallen
        .a_rising = {&states[3], NULL},
        .a_falling = {&states[2], NULL},
        .b_rising = {&states[1], NULL},
        .b_falling = {&states[2], NULL}
    },
    { // A risen B fallen
        .a_rising = {&states[3], NULL},
        .a_falling = {&states[2], NULL},
        .b_rising = {&states[0], transition_clockwise_end},
        .b_falling = {&states[3], NULL}
    },
};
struct state* current_state = &states[0];

struct button_state_machine* rotary_button_state_machine = NULL;

// __attribute__ thing is needed because I don't need data and the compiler was complaining
static void detect_turns(int num_events, lgGpioAlert_p events, void *data __attribute__((unused)))
{
    for(int i = 0; i < num_events; i++) {
        struct lgGpioAlert_s* event = events + i;
        // check if it's a reading from the rotary encoder because lgGpioSetSamplesFunc gets everything
        bool correct_chip = event->report.chip == CHIP_2;
        bool correct_gpio = event->report.gpio == ROTARY_ENCODER_A_GPIO || event->report.gpio == ROTARY_ENCODER_B_GPIO;
        if(!(correct_chip && correct_gpio)) {
            continue;
        }

        // determine event type
        bool is_rising = event->report.level == 1;
        struct state_event* state_event = NULL;
        if(event->report.gpio == ROTARY_ENCODER_A_GPIO) {
            if (is_rising) {
                state_event = &current_state->a_rising;
            } else {
                state_event = &current_state->a_falling;
            }
        } else {
            if (is_rising) {
                state_event = &current_state->b_rising;
            } else {
                state_event = &current_state->b_falling;
            }
        }

        // transition states
        if(state_event->action != NULL) {
            state_event->action();
        }
        current_state = state_event->next_state;
    }
}

static void state_machine_on_press(long curr_time, struct button_state_machine* state_machine)
{
    if(curr_time - state_machine->last_press_time > BUTTON_DEBOUNCE) {
        on_press_listener();
        state_machine->last_press_time = curr_time;
    }
}

static void detect_press(int num_events, lgGpioAlert_p events, void *data __attribute__((unused)))
{
    for(int i = 0; i < num_events; i++) {
        struct lgGpioAlert_s* event = events + i;
        // check if it's a reading from the rotary encoder because lgGpioSetSamplesFunc gets everything
        bool correct_chip = event->report.chip == CHIP_0;
        bool correct_gpio = event->report.gpio == ROTARY_ENCODER_BUTTON_GPIO;
        if(!(correct_chip && correct_gpio)) {
            continue;
        }

        bool is_rising = event->report.level == 1;
        button_state_machine_update(rotary_button_state_machine, is_rising, time_ms());
    }
}

int rotary_encoder_init(void)
{
    assert(!initialized);
    chip_2_handle = lgGpiochipOpen(CHIP_2);
    if(chip_2_handle < 0) {
        printf("chip 2 failed to open code %d", chip_2_handle);
        return 1;
    }

    chip_0_handle = lgGpiochipOpen(CHIP_0);
    if(chip_0_handle < 0) {
        printf("chip 0 failed to open code %d", chip_0_handle);
        return 2;
    }

    rotary_button_state_machine = button_state_machine_init(state_machine_on_press);

    int code = lg_gpio_samples_func_add(detect_press);
    if(code) {
        return 3;
    }
    code = lg_gpio_samples_func_add(detect_turns);
        if(code) {
        return 4;
    }

    code = lgGpioClaimAlert(chip_2_handle, 0, LG_BOTH_EDGES, ROTARY_ENCODER_A_GPIO, -1);
    if(code != 0) {
        printf("failed to claim alert encoder A %d", code);
        return 5;
    }
    code = lgGpioClaimAlert(chip_2_handle, 0, LG_BOTH_EDGES, ROTARY_ENCODER_B_GPIO, -1);
    if(code != 0) {
        printf("failed to claim alert encoder B %d", code);
        return 6;
    }
    code = lgGpioClaimAlert(chip_0_handle, 0, LG_BOTH_EDGES, ROTARY_ENCODER_BUTTON_GPIO, -1);
    if(code != 0) {
        printf("failed to claim alert encoder button %d", code);
        return 7;
    }

    initialized = true;
    return 0;
}

void rotary_encoder_set_press_listener(void (* on_press)())
{
    assert(initialized);
    if(on_press == NULL) {
        on_press_listener = do_nothing;
    } else {
        on_press_listener = on_press;
    }
}

// bool argument is true for clockwise, false for counter clockwise
void rotary_encoder_set_turn_listener(void (* on_turn)(bool))
{
    assert(initialized);
    if(on_turn == NULL) {
        on_turn_listener = do_nothing_b;
    } else {
        on_turn_listener = on_turn;
    }
}

void rotary_encoder_cleanup(void)
{
    assert(initialized);
    lg_gpio_samples_func_remove(detect_press);
    lg_gpio_samples_func_remove(detect_turns);

    lgGroupFree(chip_2_handle, ROTARY_ENCODER_A_GPIO);
    lgGroupFree(chip_2_handle, ROTARY_ENCODER_B_GPIO);
    lgGroupFree(chip_0_handle, ROTARY_ENCODER_BUTTON_GPIO);
    lgGpioFree(chip_2_handle, ROTARY_ENCODER_A_GPIO);
    lgGpioFree(chip_2_handle, ROTARY_ENCODER_B_GPIO);
    lgGpioFree(chip_0_handle, ROTARY_ENCODER_BUTTON_GPIO);
    lgGpiochipClose(chip_2_handle);
    lgGpiochipClose(chip_0_handle);
    chip_2_handle = -1;

    button_state_machine_free(&rotary_button_state_machine);
    on_turn_listener = do_nothing_b;
    on_press_listener = do_nothing;
    initialized = false;
}


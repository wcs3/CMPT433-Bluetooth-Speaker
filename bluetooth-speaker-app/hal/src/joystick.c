#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <byteswap.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "lgpio.h"
#include "hal/time_util.h"
#include "hal/i2c-init.h"
#include "hal/joystick.h"
#include "hal/button_state_machine.h"
#include "hal/lg_gpio_samples_func.h"

static const int CHIP_2 = 2;

static const char* JOYSTICK_BUS_PATH = "/dev/i2c-1";
static const uint8_t JOYSTICK_ADDRESS = 0x48;
static const uint16_t JOYSTICK_Y_SAMPLE = 0x83C2;
static const uint16_t JOYSTICK_X_SAMPLE = 0x83D2;

static const long BUTTON_DEBOUNCE = 50;

static const uint16_t JOYSTICK_MIN = 200; //60;
static const uint16_t JOYSTICK_MAX = 26000; // 26256;
static const uint16_t JOYSTICK_CENTER = 13100;
static const uint16_t JOYSTICK_DEADZONE = 550;
static const uint16_t JOYSTICK_DEADZONE_MIN = JOYSTICK_CENTER - JOYSTICK_DEADZONE;
static const uint16_t JOYSTICK_DEADZONE_MAX = JOYSTICK_CENTER + JOYSTICK_DEADZONE;
static const uint16_t JOYSTICK_DEADZONE_TO_MIN = JOYSTICK_DEADZONE_MIN - JOYSTICK_MIN;
static const uint16_t JOYSTICK_DEADZONE_TO_MAX = JOYSTICK_MAX - JOYSTICK_DEADZONE_MAX;

static const float JOYSTICK_UP_TRIGGER = 0.9f;
static const float JOYSTICK_UP_HYSTERESIS = 0.5f;
static const float JOYSTICK_DOWN_TRIGGER = -0.9f;
static const float JOYSTICK_DOWN_HYSTERESIS = -0.5f;
static const int SAMPLE_SLEEP_TIME = 50;
static const long HOLD_REPEAT_TIME = 500;

static _Atomic bool keep_sampling = false;
static pthread_t sampling_thread;
static pthread_mutex_t read_stick_lock;

static bool initalized = false;
static int joystick_file_desc = -1;

static int JOYSTICK_BUTTON_GPIO = 15;
static int chip_2_handle = -1;

static void do_nothing(void) {}
static void (* _Atomic on_press_listener)() = do_nothing;
static void (* _Atomic on_up_listener)() = do_nothing;
static void (* _Atomic on_down_listener)() = do_nothing;

static void on_press(long curr_time, struct button_state_machine* state_machine) 
{
     if(curr_time - state_machine->last_press_time > BUTTON_DEBOUNCE) {
        on_press_listener();
        state_machine->last_press_time = curr_time;
    }
}

struct button_state_machine* stick_button_state_machine = NULL;

static void* do_sampling(void* args __attribute__((unused)))
{
    int code;
    bool in_up_motion = false;
    bool in_down_motion = false;
    long last_up_trigger = time_ms();
    long last_down_trigger = last_up_trigger;

    while (keep_sampling)
    {
        float stick_y;
        code = joystick_read_y(&stick_y);
        if(code) {
            fprintf(stderr, "Failed to read joystick in sampling thread %d\n", code);
            return NULL;
        }

        long curr_time = time_ms();

        // determine if in_up_motion
        if(in_up_motion && stick_y < JOYSTICK_UP_TRIGGER - JOYSTICK_UP_HYSTERESIS) {
            in_up_motion = false;
        } else if(!in_up_motion && stick_y >= JOYSTICK_UP_TRIGGER) {
            in_up_motion = true;
            on_up_listener();
            last_up_trigger = curr_time;
        }

        // determine if in_down_motion
        if(in_down_motion && stick_y > JOYSTICK_DOWN_TRIGGER - JOYSTICK_DOWN_HYSTERESIS) {
            in_down_motion = false;
        } else if(!in_down_motion && stick_y <= JOYSTICK_DOWN_TRIGGER) {
            in_down_motion = true;
            on_down_listener();
            last_down_trigger = curr_time;
        }

        // repeat listeners if held
        if(in_up_motion && curr_time - last_up_trigger > HOLD_REPEAT_TIME) {
            on_up_listener();
            last_up_trigger = curr_time;
        }

        if(in_down_motion && curr_time - last_down_trigger > HOLD_REPEAT_TIME) {
            on_down_listener();
            last_down_trigger = curr_time;
        }

        sleep_ms(SAMPLE_SLEEP_TIME);
    }
    return NULL;
}

static void detect_presses(int num_events, lgGpioAlert_p events, void *data __attribute__((unused)))
{
    for(int i = 0; i < num_events; i++) {
        struct lgGpioAlert_s* event = events + i;
        // check if it's a reading from the rotary encoder because lgGpioSetSamplesFunc gets everything
        bool correct_chip = event->report.chip == CHIP_2;
        bool correct_gpio = event->report.gpio == JOYSTICK_BUTTON_GPIO;
        if(!(correct_chip && correct_gpio)) {
            continue;
        }

        // determine event type
        bool is_rising = event->report.level == 1;
        button_state_machine_update(stick_button_state_machine, is_rising, time_ms());
    }
}

int joystick_init(void)
{
    assert(!initalized);

    // i2c
    int code = init_i2c_bus(JOYSTICK_BUS_PATH, JOYSTICK_ADDRESS, &joystick_file_desc);
    if(code) {
        fprintf(stderr, "Failed to init bus in joystick %d\n", code);
        return 1;
    }
    initalized = true;

    // thread
    code = pthread_mutex_init(&read_stick_lock, NULL);
    if(code != 0) {
        fprintf(stderr, "Failed to init mutex in joystick %d\n", code);
        return 2;
    }

    // mutex
    keep_sampling = true;
    code = pthread_create(&sampling_thread, NULL, do_sampling, NULL);
    if(code) {
        fprintf(stderr, "Failed to init thread in joystick %d\n", code);
        return 3;
    }

    // gpio (button)
    // curr_state = &states[0];
    stick_button_state_machine = button_state_machine_init(on_press);
    chip_2_handle = lgGpiochipOpen(CHIP_2);
    if(chip_2_handle < 0) {
        printf("chip 2 failed to open code %d", chip_2_handle);
        return 4;
    }

    code = lg_gpio_samples_func_add(detect_presses);
    if(code) {
        return 5;
    }
    code = lgGpioClaimAlert(chip_2_handle, 0, LG_BOTH_EDGES, JOYSTICK_BUTTON_GPIO, -1);
    if(code != 0) {
        printf("failed to claim alert joystick button gpio %d", code);
        return 6;
    }
    
    return 0;
}

void joystick_set_on_press_listener(void (*on_press)())
{
    assert(initalized);
    if(on_press == NULL) {
        on_press_listener = do_nothing;
    } else {
        on_press_listener = on_press;
    }
}
void joystick_set_on_up_listener(void (*on_up)())
{
    assert(initalized);
    if(on_up == NULL) {
        on_up_listener = do_nothing;
    } else {
        on_up_listener = on_up;
    }
}
void joystick_set_on_down_listener(void (*on_down)())
{
    assert(initalized);
    if(on_down == NULL) {
        on_down_listener = do_nothing;
    } else {
        on_down_listener = on_down;
    }
}

int joystick_read_x(float* result) 
{
    assert(initalized);
    pthread_mutex_lock(&read_stick_lock);
    int error_code =  write_i2c_reg16(joystick_file_desc, 1, JOYSTICK_X_SAMPLE);
    if(error_code) {
        return 1;
    }

    uint16_t res_x;
    error_code = read_i2c_reg16(joystick_file_desc, 0, &res_x);
    if(error_code) {
        return 2;
    }
    pthread_mutex_unlock(&read_stick_lock);
    res_x = bswap_16(res_x);

    if(res_x < JOYSTICK_DEADZONE_MIN) {
        *result = -((JOYSTICK_DEADZONE_MIN - res_x) /((float) JOYSTICK_DEADZONE_TO_MIN));
        if(*result < -1.0f) {
            *result = -1.0f;
        }
    } else if(res_x > JOYSTICK_DEADZONE_MAX) {
        *result = (res_x - JOYSTICK_DEADZONE_MAX) /((float) JOYSTICK_DEADZONE_TO_MAX);
        if(*result > 1.0f) {
            *result = 1.0f;
        }
    } else {
        *result = 0.0f;
    }
    return 0;
}

int joystick_read_y(float* result) 
{
    assert(initalized);
    pthread_mutex_lock(&read_stick_lock);
    int error_code =  write_i2c_reg16(joystick_file_desc, 1, JOYSTICK_Y_SAMPLE);
    if(error_code) {
        return 3;
    }

    uint16_t res_y;
    error_code = read_i2c_reg16(joystick_file_desc, 0, &res_y);
    if(error_code) {
        return 4;
    }
    pthread_mutex_unlock(&read_stick_lock);
    res_y = bswap_16(res_y);

    if(res_y < JOYSTICK_DEADZONE_MIN) {
        *result = -((JOYSTICK_DEADZONE_MIN - res_y) /((float) JOYSTICK_DEADZONE_TO_MIN));
        if(*result < -1.0f) {
            *result = -1.0f;
        }
    } else if(res_y > JOYSTICK_DEADZONE_MAX) {
        *result = (res_y - JOYSTICK_DEADZONE_MAX) /((float) JOYSTICK_DEADZONE_TO_MAX);
        if(*result > 1.0f) {
            *result = 1.0f;
        }
    } else {
        *result = 0.0f;
    }

    return 0;
}

int joystick_read_xy(struct joystick_position* result) {
    assert(initalized);
    int code = joystick_read_x(&result->x);
    if(code) {
        return code;
    }
    code = joystick_read_y(&result->y);
    if(code) {
        return code;
    }
    return 0;
}

void joystick_cleanup(void)
{
    assert(initalized);
    // stop thread
    int code;
    keep_sampling = false;
    code = pthread_join(sampling_thread, NULL);
    if(code != 0) {
        fprintf(stderr, "Failed to join thread in joystick %d\n", code);
    }

    // i2c
    close_i2c_bus(joystick_file_desc);
    joystick_file_desc = -1;

    // mutex
    code = pthread_mutex_destroy(&read_stick_lock);
    if(code != 0) {
        fprintf(stderr, "Failed to destroy mutex in joystick %d\n", code);
    }

    // gpio
    lg_gpio_samples_func_remove(detect_presses);
    lgGroupFree(chip_2_handle, JOYSTICK_BUTTON_GPIO);
    lgGpioFree(chip_2_handle, JOYSTICK_BUTTON_GPIO);
    lgGpiochipClose(chip_2_handle);
    chip_2_handle = -1;
    button_state_machine_free(&stick_button_state_machine);

    on_up_listener = do_nothing;
    on_down_listener = do_nothing;
    on_press_listener = do_nothing;

    initalized = false;
}
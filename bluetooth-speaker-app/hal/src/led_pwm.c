#define _POSIX_C_SOURCE 200809L
#include <sys/time.h>
#include "hal/light_sensor.h"
#include "hal/i2c-init.h"
#include "lgpio.h"

#include <stdlib.h>
#include <byteswap.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

struct light_sensor_sample_and_time {
    uint16_t reading;
    long time;
};

static const char* ENABLE_PATH = "/dev/hat/pwm/GPIO12/enable";
static const char* PERIOD_PATH = "/dev/hat/pwm/GPIO12/period";
static const char* DUTY_CYCLE_PATH = "/dev/hat/pwm/GPIO12/duty_cycle";

static const long NANOSECONDS_IN_SECOND = 1000000000;
static const int STARTING_HERTZ = 0;
static bool initalized = false;

static int current_hertz = 0;

// 0 if successful, 1 if error opening, 2 if error closing
static int write_number(const char* file_name, long number)
{
    FILE* file = fopen(file_name, "w");
    if(file == NULL) {
        return 1;
    }
    fprintf(file, "%ld\n", number);
    if(fclose(file)) {
        return 2;
    }
    return 0;
}

int led_pwm_set_hertz(int new_hertz)
{
    // because the period goes up to 469,754,879 ns
    assert(new_hertz >= 3 || new_hertz == 0);
    if(new_hertz == 0) {
        // failing to write is OK, it happens if it is already 0
        int code = write_number(ENABLE_PATH, 0);
        if(code == 1) {
            return 2;
        }
        current_hertz = 0;
    } else {
        long period = NANOSECONDS_IN_SECOND / new_hertz;
        long duty_cycle = period / 2;

        // failing to write is OK, it happens if it is already 0
        int code = write_number(ENABLE_PATH, 0);
        if(code == 1) {
            return 2;
        }
        code = write_number(PERIOD_PATH, period);
        if(code) {
            return 3;
        }
        code = write_number(DUTY_CYCLE_PATH, duty_cycle);
        if(code) {
            return 4;
        }
        code = write_number(ENABLE_PATH, 1);
        if(code) {
            return 5;
        }
        current_hertz = new_hertz;
    }
    
    return 0;
}

int led_pwm_init()
{
    assert(!initalized);
    current_hertz = STARTING_HERTZ;

    write_number(ENABLE_PATH, 0);
    write_number(DUTY_CYCLE_PATH, 0);
    write_number(PERIOD_PATH, 0);

    int code = led_pwm_set_hertz(current_hertz);
    if(code) {
        return code;
    }

    initalized = true;
    return 0;    
}

int led_pwm_get_hertz()
{
    return current_hertz;
}

void led_pwm_cleanup()
{
    assert(initalized);
    write_number(ENABLE_PATH, 0);
    write_number(DUTY_CYCLE_PATH, 0);
    write_number(PERIOD_PATH, 0);
    initalized = false;
}
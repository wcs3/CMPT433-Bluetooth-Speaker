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

#include "hal/i2c-init.h"
#include "hal/joystick.h"

const char* JOYSTICK_BUS_PATH = "/dev/i2c-1";
const uint8_t JOYSTICK_ADDRESS = 0x48;
const uint16_t JOYSTICK_Y_SAMPLE = 0x83C2;
const uint16_t JOYSTICK_X_SAMPLE = 0x83D2;
const uint16_t JOYSTICK_LED_SAMPLE = 0x83E2;
const uint16_t JOYSTICK_ADC_SAMPLE = 0x83F2;

const uint16_t JOYSTICK_MIN = 200; //60;
const uint16_t JOYSTICK_MAX = 26000; // 26256;
const uint16_t JOYSTICK_CENTER = 13100;
const uint16_t JOYSTICK_DEADZONE = 550;
const uint16_t JOYSTICK_DEADZONE_MIN = JOYSTICK_CENTER - JOYSTICK_DEADZONE;
const uint16_t JOYSTICK_DEADZONE_MAX = JOYSTICK_CENTER + JOYSTICK_DEADZONE;
const uint16_t JOYSTICK_DEADZONE_TO_MIN = JOYSTICK_DEADZONE_MIN - JOYSTICK_MIN;
const uint16_t JOYSTICK_DEADZONE_TO_MAX = JOYSTICK_MAX - JOYSTICK_DEADZONE_MAX;

static bool initalized = false;
static int joystick_file_desc = 0;

int joystick_init(void)
{
    int error_code = init_i2c_bus(JOYSTICK_BUS_PATH, JOYSTICK_ADDRESS, &joystick_file_desc);
    if(error_code) {
        return 1;
    }

    initalized = true;
    return 0;
}

int joystick_read_x(float* result) 
{
    int error_code =  write_i2c_reg16(joystick_file_desc, 1, JOYSTICK_X_SAMPLE);
    if(error_code) {
        return 1;
    }

    uint16_t res_x;
    error_code = read_i2c_reg16(joystick_file_desc, 0, &res_x);
    if(error_code) {
        return 2;
    }
    res_x = bswap_16(res_x);
    // printf("x: %d\n", res_x);

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
    int error_code =  write_i2c_reg16(joystick_file_desc, 1, JOYSTICK_Y_SAMPLE);
    if(error_code) {
        return 3;
    }

    uint16_t res_y;
    error_code = read_i2c_reg16(joystick_file_desc, 0, &res_y);
    if(error_code) {
        return 4;
    }
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
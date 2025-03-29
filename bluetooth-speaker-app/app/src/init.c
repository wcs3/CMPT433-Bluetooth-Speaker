#define _POSIX_C_SOURCE 200809L
#include "app/init.h"
#include "hal/rotary_encoder.h"
#include "hal/light_sensor.h"
#include "hal/led_pwm.h"
#include "hal/draw_stuff.h"
#include "hal/period_timer.h"
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

static _Atomic bool shutdown = false;
static pthread_barrier_t barrier;
// pthread_barrier_t with count = 1 not allowed
static bool num_confirms_0 = false;

int init_start(int num_confirms)
{
    int code;
    Period_init();

    // code = light_sensor_init();
    // if(code) {
    //     fprintf(stderr, "failed to init light sensor %d\n", code);
    //     return 2;
    // }

    // code = rotary_encoder_init();
    // if(code) {
    //     fprintf(stderr, "failed to init rotary encoder %d\n", code);
    //     return 3;
    // }

    // code = led_pwm_init();
    // if(code) {
    //     fprintf(stderr, "failed to init pwm led %d\n", code);
    //     return 4;
    // }

    // draw_stuff_init();
    if(num_confirms > 0) {
        code = pthread_barrier_init(&barrier, NULL, num_confirms + 1);
        if(code) {
            fprintf(stderr, "failed to init barrier %d\n", code);
            return 5;
        }
        num_confirms_0 = false;
    } else {
        num_confirms_0 = true;
    }
    
    return 0;
}

void init_set_shutdown()
{
    shutdown = true;
}

bool init_get_shutdown()
{
    return shutdown;
}

void init_signal_done()
{
    if(num_confirms_0) {
        return;
    }
    int code = pthread_barrier_wait(&barrier);
    if(code != PTHREAD_BARRIER_SERIAL_THREAD && code != 0) {
        fprintf(stderr, "failed to wait barrier init_signal_done %d\n", code);
    }
}

void init_end(void) 
{
    int code;
    if(!num_confirms_0) {
        code = pthread_barrier_wait(&barrier);
        if(code != PTHREAD_BARRIER_SERIAL_THREAD && code != 0) {
            fprintf(stderr, "failed to wait barrier init_end %d\n", code);
        }
    }
    
    // draw_stuff_cleanup();
    // light_sensor_cleanup();
    // rotary_encoder_cleanup();
    // led_pwm_cleanup();
    // Period_cleanup();
    if(!num_confirms_0) {
        code = pthread_barrier_destroy(&barrier);
        if(code) {
            fprintf(stderr, "failed to destroy barrier %d\n", code);
        }
    }
}
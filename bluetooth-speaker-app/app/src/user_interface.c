// handles terminal output, LCD, rotary encoder controls
#include "app/user_interface.h"
#include "app/init.h"
#include "hal/rotary_encoder.h"
#include "hal/led_pwm.h"
#include "hal/light_sensor.h"
#include "hal/draw_stuff.h"
#include "hal/period_timer.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const int MAX_FREQUENCY = 500;
static const int MIN_FLASH_FREQUENCY = 3;
static const int NUM_SAMPLES_DISPLAY = 10;

static pthread_t ui_thread;

static void on_turn(bool clockwise)
{
    int frequency = led_pwm_get_hertz();
    // avoids setting hertz to 1 and 2 for hardware limitation reasons
    if(clockwise) {
        if(frequency == 0) {
            led_pwm_set_hertz(MIN_FLASH_FREQUENCY);
        } else if(frequency < MAX_FREQUENCY) {
            led_pwm_set_hertz(frequency + 1);
        }
    } else {
        if(frequency > 3) {
            led_pwm_set_hertz(frequency - 1);
        } else {
            led_pwm_set_hertz(0);
        }
    }
}

void* run_ui(void* arg __attribute__((unused)))
{
    while(!init_get_shutdown()) {
        // summary stats
        struct light_sensor_statistics light_stats;
        light_sensor_history_1sec(&light_stats);
        int num_dips = light_sensor_count_dips(&light_stats);

        Period_statistics_t period_stats;
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &period_stats);

        printf(
            "#Smpl/s = %d Flash = %d avg = %f dips = %d Smpl ms[ %f, %f] avg %f/%d\n",
            light_stats.size,
            led_pwm_get_hertz(),
            light_stats.average,
            num_dips,
            period_stats.minPeriodInMs,
            period_stats.maxPeriodInMs,
            period_stats.avgPeriodInMs,
            period_stats.numSamples
        );

        // evenly spaced readings over last second
        if(light_stats.size <= NUM_SAMPLES_DISPLAY) {
            for(int i = 0; i < NUM_SAMPLES_DISPLAY; i++) {
                printf("%d:%.3f ", i, light_stats.readings[i]);
            }
        } else {
            int step = light_stats.size / NUM_SAMPLES_DISPLAY;
            for(int i = 0; i < NUM_SAMPLES_DISPLAY; i++) {
                printf("%d:%.3f ", i * step, light_stats.readings[i * step]);
            }
        }
        printf("\n");

        // lcd message
        char buf[128];
        snprintf(buf, 128, "Maxwell's\nFlash = %d\nDips = %d\nMax ms = %f", led_pwm_get_hertz(), num_dips, period_stats.maxPeriodInMs);
        draw_stuff_update_screen(buf);

        light_sensor_statistics_free(&light_stats);
        sleep(1);
    }

    init_signal_done();
    return NULL;
}

int user_interface_init()
{
    led_pwm_set_hertz(10);
    rotary_encoder_set_turn_listener(on_turn);
    int thread_code = pthread_create(&ui_thread, NULL, run_ui, NULL);
    if(thread_code) {
        fprintf(stderr, "failed to create ui thread %d\n", thread_code);
        return 1;
    }
    return 0;
}

void user_interface_cleanup()
{
    int thread_code = pthread_join(ui_thread, NULL);
    if(thread_code) {
        fprintf(stderr, "failed to join ui thread %d\n", thread_code);
    }
}
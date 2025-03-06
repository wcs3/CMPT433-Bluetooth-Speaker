#define _POSIX_C_SOURCE 200809L
#include <sys/time.h>

#include "hal/light_sensor.h"
#include "hal/i2c-init.h"
#include "hal/period_timer.h"

#include <stdlib.h>
#include <byteswap.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

struct light_sensor_sample_and_time {
    double reading;
    long time;
};

// the max voltage is 2.048
const double MAX_VOLTAGE = 2.048;
const int LIGHT_SENSOR_MAX_VOLTAGE_DIGITS = 1;

// i2c
static const char* ENABLE_PATH = "/dev/i2c-1";
static const uint8_t ADC_ADDRESS = 0x48;

// O: OS, M: mux, P: PGA, M: mode, D: data rate, R : reserved
// OMMM PPPM DDDR RRRR
// 1110 0010 1000 0011
const uint16_t LIGHT_SENSOR_SAMPLE = 0x83E2;

// collecting stats
const int BUFFER_SIZE = 1500;
static const long ONE_SECOND_MS = 1000;
static const double EXP_AVERAGE_ALPHA = 0.01;

// adc conversion related
static const double FSR = 4.096;
static const int ADC_RESOLUTION = 4096; // 2^12

// dip analysis
static const double DIP_VOLTAGE_DROP = 0.1;
static const double DIP_HYSTERESIS = 0.03;

static int adc_file_desc = 0;
static bool initalized = false;

static pthread_t sampling_thread;
static pthread_mutex_t buffer_lock;

static _Atomic bool keep_sampling = false;
static struct light_sensor_sample_and_time* sample_buffer = NULL;
static _Atomic long sample_count = 0;
static double exp_reading_average = 0.0;

// takes in raw register reading
static double adc_reading_to_voltage(int16_t adc_reading)
{
    adc_reading = bswap_16(adc_reading);
    // 4 lsb are always 0
    adc_reading = adc_reading >> 4;
    return adc_reading * (FSR / ADC_RESOLUTION);
}

// https://opencoursehub.cs.sfu.ca/bfraser/grav-cms/cmpt433/assignments/files/Assignment1.pdf
static long time_ms(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long seconds = spec.tv_sec;
    long nanoSeconds = spec.tv_nsec;
    long milliSeconds = seconds * 1000 + nanoSeconds / 1000000;
    return milliSeconds;
}

static int sleep_ms(long miliseconds)
{
    struct timespec req;
    req.tv_nsec = (miliseconds % 1000) * 1000000;
    req.tv_sec = miliseconds / 1000;
    return nanosleep(&req, NULL);
}

static int light_sensor_sample(double* result)
{
    int error_code = write_i2c_reg16(adc_file_desc, 1, LIGHT_SENSOR_SAMPLE);
    if(error_code) {
        return 1;
    }

    uint16_t result_raw;
    error_code = read_i2c_reg16(adc_file_desc, 0, &result_raw);
    if(error_code) {
        return 2;
    }
    *result = adc_reading_to_voltage(result_raw);
    return 0;
}

static void* do_sampling(void* args __attribute__((unused)))
{
    // init average
    light_sensor_sample(&exp_reading_average);
    while(keep_sampling) {
        struct light_sensor_sample_and_time sample;
        light_sensor_sample(&sample.reading);
        sample.time = time_ms();

        pthread_mutex_lock(&buffer_lock);
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        sample_buffer[sample_count % BUFFER_SIZE] = sample;
        sample_count++;
        exp_reading_average = sample.reading * EXP_AVERAGE_ALPHA + exp_reading_average * (1.0 - EXP_AVERAGE_ALPHA);
        pthread_mutex_unlock(&buffer_lock);
        
        sleep_ms(1);
    }
    return NULL;
}

int light_sensor_init(void)
{
    assert(!initalized);
    int error_code = init_i2c_bus(ENABLE_PATH, ADC_ADDRESS, &adc_file_desc);
    if(error_code) {
        return 1;
    }

    sample_buffer= malloc(sizeof(sample_buffer[0]) * BUFFER_SIZE);
    for(int i = 0; i < BUFFER_SIZE; i++) {
        sample_buffer[i].reading = 0;
        sample_buffer[i].time = 0;
    }

    keep_sampling = true;
    pthread_create(&sampling_thread, NULL, do_sampling, NULL);
    pthread_mutex_init(&buffer_lock, NULL);

    initalized = true;
    return 0;
}

void light_sensor_cleanup(void)
{
    assert(initalized);
    keep_sampling = false;
    pthread_join(sampling_thread, NULL);
    close_i2c_bus(adc_file_desc);

    free(sample_buffer);
    keep_sampling = false;
    sample_buffer = NULL;
    sample_count = 0;
    adc_file_desc = 0;
    initalized = false;
    pthread_mutex_destroy(&buffer_lock);
}

void light_sensor_history_1sec(struct light_sensor_statistics* stats)
{
    stats->readings = malloc(sizeof(double) * BUFFER_SIZE);
    stats->size = 0;
    stats->min = MAX_VOLTAGE;
    stats->max = 0.0;

    pthread_mutex_lock(&buffer_lock);
    stats->average = exp_reading_average;
    // get samples taken less than 1 second ago
    long curr_time_ms = time_ms();
    for(int i = 0; i < BUFFER_SIZE; i++) {
        struct light_sensor_sample_and_time sample = sample_buffer[(sample_count + i) % BUFFER_SIZE];
        if(sample.time >= (curr_time_ms - ONE_SECOND_MS)) {
            stats->readings[stats->size] = sample.reading;
            stats->size++;

            if(sample.reading > stats->max) {
                stats->max = sample.reading;
            }
            if(sample.reading < stats->min) {
                stats->min = sample.reading;
            }
        }
    }
    pthread_mutex_unlock(&buffer_lock);
}

void light_sensor_statistics_free(struct light_sensor_statistics* stats)
{
    free(stats->readings);
}

long light_sensor_samples_taken()
{
    return sample_count;
}

double light_sensor_average()
{
    pthread_mutex_lock(&buffer_lock);
    double result = exp_reading_average;
    pthread_mutex_unlock(&buffer_lock);
    return result;
}

int light_sensor_count_dips(struct light_sensor_statistics* stats)
{
    int dip_count = 0;
    bool in_dip = false;
    for(int i = 0; i < stats->size; i++) {
        double reading = stats->readings[i];
        if(in_dip) {
            if(reading > stats->average - DIP_VOLTAGE_DROP + DIP_HYSTERESIS) {
                in_dip = false;
            }
            // else still in a dip
        } else {
            if(reading < stats->average - DIP_VOLTAGE_DROP) {
                in_dip = true;
                dip_count++;
            }
            // else still no dip
        }
    }

    return dip_count;
}
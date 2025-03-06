// Starts a thread that continuously samples light levels.
#ifndef _LIGHT_SENSOR_H_
#define _LIGHT_SENSOR_H_

#include <stdint.h>

struct light_sensor_statistics {
    double* readings;
    int size;
    double average;
    double min;
    double max;
};

extern const int LIGHT_SENSOR_MAX_VOLTAGE_DIGITS;

// Starts sampling thread
int light_sensor_init(void);
void light_sensor_history_1sec(struct light_sensor_statistics* stats);
void light_sensor_statistics_free(struct light_sensor_statistics* stats);
int light_sensor_count_dips(struct light_sensor_statistics* stats);
long light_sensor_samples_taken();
double light_sensor_average();
// Stops sampling thread
void light_sensor_cleanup(void);
#endif
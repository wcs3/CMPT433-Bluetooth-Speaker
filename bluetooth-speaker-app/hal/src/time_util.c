#define _POSIX_C_SOURCE 200809L
#include "hal/time_util.h"
#include <time.h>

long time_ms(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long seconds = spec.tv_sec;
    long nanoSeconds = spec.tv_nsec;
    long milliSeconds = seconds * 1000 + nanoSeconds / 1000000;
    return milliSeconds;
}

int sleep_ms(long miliseconds)
{
    struct timespec req;
    req.tv_nsec = (miliseconds % 1000) * 1000000;
    req.tv_sec = miliseconds / 1000;
    return nanosleep(&req, NULL);
}

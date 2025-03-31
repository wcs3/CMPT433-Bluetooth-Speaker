#include "hal/lg_gpio_samples_func.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#define MAX_NUM_FUNCS 10

static lgGpioAlertsFunc_t funcs[MAX_NUM_FUNCS];
static int num_funcs = 0;
static pthread_mutex_t funcs_lock;
static bool initialized = false;

static void samples(int num_events, lgGpioAlert_p events, void *data)
{
    pthread_mutex_lock(&funcs_lock);
    for(int i = 0; i < MAX_NUM_FUNCS; i++) {
        if(funcs[i] != NULL) {
            funcs[i](num_events, events, data);
        }
    }
    pthread_mutex_unlock(&funcs_lock);
}

int lg_gpio_samples_func_init(void)
{
    assert(!initialized);
    int code = pthread_mutex_init(&funcs_lock, NULL);
    if(code != 0) {
        fprintf(stderr, "lg_gpio_samples_func mutex create failed %d\n", code);
    }

    num_funcs = 0;
    for(int i = 0; i < MAX_NUM_FUNCS; i++) {
        funcs[i] = NULL;
    }
    lgGpioSetSamplesFunc(samples, NULL);

    initialized = true;
    return 0;
}
int lg_gpio_samples_func_add(lgGpioAlertsFunc_t samples_func)
{
    // insert the function at the end of funcs if there is space, not going to bother supporting lots of adds and removes
    assert(initialized);
    pthread_mutex_lock(&funcs_lock);

    if(num_funcs == MAX_NUM_FUNCS) {
        fprintf(stderr, "Max number of samples functions reached\n");
        pthread_mutex_unlock(&funcs_lock);
        return 1;
    }
    
    funcs[num_funcs] = samples_func;
    num_funcs++;
    pthread_mutex_unlock(&funcs_lock);

    return 0;
}

int lg_gpio_samples_func_remove(lgGpioAlertsFunc_t samples_func)
{
    // just null out the function to remove, not going to bother supporting lots of adds and removes
    assert(initialized);
    pthread_mutex_lock(&funcs_lock);
    for(int i = 0; i < num_funcs; i++) {
        if(funcs[i] == samples_func) {
            pthread_mutex_unlock(&funcs_lock);
            funcs[i] = NULL;
            return 0;
        }
    }
    pthread_mutex_unlock(&funcs_lock);
    return 1;
}

void lg_gpio_samples_func_cleanup(void)
{
    assert(initialized);
    lgGpioSetSamplesFunc(NULL, NULL);
    num_funcs = 0;
    for(int i = 0; i < MAX_NUM_FUNCS; i++) {
        funcs[i] = NULL;
    }
    pthread_mutex_destroy(&funcs_lock);
    initialized = false;
}
// Allow for multiple hals to get lgpio events
#ifndef _LG_GPIO_SAMPLE_FUNC_H
#define _LG_GPIO_SAMPLE_FUNC_H

#include "lgpio.h"

int lg_gpio_samples_func_init(void);

// add a function that receives lgpio events, returns 0 if successful, not 0 otherwise
int lg_gpio_samples_func_add(lgGpioAlertsFunc_t samples_func);

// remove a function that receives lgpio events, returns 0 if successful, not 0 otherwise
int lg_gpio_samples_func_remove(lgGpioAlertsFunc_t samples_func);

void lg_gpio_samples_func_cleanup(void);

#endif
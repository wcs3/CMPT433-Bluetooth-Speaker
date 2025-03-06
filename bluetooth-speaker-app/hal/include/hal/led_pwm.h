// Making the led flash at a specified frequency
#ifndef _LED_PWM_H
#define _LED_PWM_H

int led_pwm_init();
int led_pwm_set_hertz(int new_hertz);
int led_pwm_get_hertz();
void led_pwm_cleanup();

#endif
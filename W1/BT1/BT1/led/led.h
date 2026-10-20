#ifndef LED_H
#define LED_H
#include <stdint.h>

void led_init(void);
void led_on(uint8_t led);      // led = 0 or 1
void led_off(uint8_t led);
void led_toggle(uint8_t led);

#endif

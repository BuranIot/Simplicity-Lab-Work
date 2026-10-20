#include "led.h"
#include "em_gpio.h"

#define LED0_PORT gpioPortD
#define LED0_PIN    2
#define LED1_PORT gpioPortD
#define LED1_PIN    3

void led_init(void){
    GPIO_PinModeSet(LED0_PORT, LED0_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(LED1_PORT, LED1_PIN, gpioModePushPull, 0);
}

void led_on(uint8_t led){
    if (led == 0) {
        GPIO_PinOutSet(LED0_PORT, LED0_PIN);
    } else {
        GPIO_PinOutSet(LED1_PORT, LED1_PIN);
    }
}

void led_off(uint8_t led){
    if (led == 0) {
        GPIO_PinOutClear(LED0_PORT, LED0_PIN);
    } else {
        GPIO_PinOutClear(LED1_PORT, LED1_PIN);
    } 
}

void led_toggle(uint8_t led){
    if (led == 0) {
        GPIO_PinOutToggle(LED0_PORT, LED0_PIN);
    } else {
        GPIO_PinOutToggle(LED1_PORT, LED1_PIN);
    }     
}

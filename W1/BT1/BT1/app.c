/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include "uart/uart.h"
#include "led/led.h"
/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
    uart_init();
    led_init();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
    uint8_t data = uart_recv_byte();   // 1. wait for a byte from PC
    switch (data) {
        case '0':
            led_on(0);
            break;
        case '1':
            led_on(1);
            break;
        case '2':
            led_off(0);
            break;
        case '3':
            led_off(1);
            break;
        case '4':
            led_toggle(0);
            break;
        case '5':
            led_toggle(1);
            break;
        case '6':
            led_on(0);
            led_on(1);
            break;
        case '7':
            led_off(0);
            led_off(1);
            break;
        default:
        // Code to run if no cases match (optional)
            uart_send_byte('?');
            break;
    }
}

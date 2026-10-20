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
#include <stdio.h>      
#include "em_cmu.h"
#include "uart/uart.h"
/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
    uart_init(115200, 8, 'N', 2, 16);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
    char buf[256];
    char send[256];

    int n = uart_recv_line(buf, sizeof(buf));
    if(n == 0){
        return;
    }
    uint32_t chars  = (uint32_t)n + 1u;     
    uint8_t  halves = uart_frame_halfbits();
    uint32_t baud   = uart_actual_baud();
    uint32_t pclk   = CMU_ClockFreqGet(cmuClock_PCLK);

    uint64_t num     = (uint64_t)chars * halves * 1000000u;
    uint32_t time_us = (uint32_t)((num + baud) / (2u * (uint64_t)baud));

    snprintf(send, sizeof(send),
             "\"%s\" | %lu chars | %u.%u bits/frame | baud %lu | PCLK %lu Hz | %lu us\r\n",
             buf,
             (unsigned long)chars,
             halves / 2u, (halves % 2u) * 5u,
             (unsigned long)baud,
             (unsigned long)pclk,
             (unsigned long)time_us);
    uart_send_string(send);
}


/***************************************************************************//**
 * @file
 * @brief Lab 4 - Bai 2: measure DMA time Memory -> UART
 *
 * PC sends one command char (read by CPU):
 *   '1' -> 10 bytes, '2' -> 100 bytes, '3' -> 1000 bytes
 * Board fills txbuf with random chars, sends it with LDMA (channel 1),
 * measures the time with TIMER0, then prints "<time> us" with the CPU.
 ******************************************************************************/
#include <stdio.h>
#include "app.h"
#include "Driver/uart/uart.h"
#include "Driver/dma/dma.h"
#include "Driver/timer/timer.h"

#define TX_MAX 1000   // must be <= DMA_MAX_XFER

static uint8_t  txbuf[TX_MAX];   // static: DMA reads it after the call returns
static uint32_t seed = 12345;

// Simple LCG pseudo-random generator (no need for stdlib rand())
static uint32_t next_rand(void)
{
  seed = seed * 1103515245u + 12345u;
  return seed >> 16;
}

// Random lowercase letters + digits, like "ahcnfue12s"
static void fill_data(uint8_t *buf, uint16_t len)
{
  static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";

  for (uint16_t i = 0; i < len; i++) {
    buf[i] = (uint8_t)chars[next_rand() % (sizeof chars - 1)];
  }
}

static uint32_t dma_send_measure(const uint8_t *buf, uint16_t len)
{
  timer_start();
  dma_uart_tx_start(buf, len);
  while (!dma_uart_tx_done()) {
    // CPU is free here; just wait for the LDMA done interrupt
  }
  return timer_stop_us();
}

static void print_help(void)
{
  uart_send_string("\r\nSend 1 (10 B), 2 (100 B), 3 (1000 B)\r\n");
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  char msg[48];

  uart_init(115200, 8, 'N', 2, 16);   // 8N1, stop_halves = 2 -> 1 stop bit
  dma_init();
  timer_init();

  uart_send_string("\r\nLab 4 - Bai 2: DMA Memory -> UART timing\r\n");
  snprintf(msg, sizeof msg, "TIMER0 clock: %lu Hz\r\n", (unsigned long)timer_clock_hz());
  uart_send_string(msg);
  print_help();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  char msg[32];
  uint16_t len;

  if (!uart_recv_ready()) {
    return;                      // no command yet
  }

  uint8_t cmd = uart_recv_byte(); // CPU reads the request

  switch (cmd) {
    case '1': len = 10;   break;
    case '2': len = 100;  break;
    case '3': len = 1000; break;
    case '\r':
    case '\n':
      return;                    // ignore Enter
    default:
      print_help();
      return;
  }

  fill_data(txbuf, len);         // prepare data BEFORE timing

  uint32_t us = dma_send_measure(txbuf, len);

  // Print AFTER the timer is stopped so printing is not measured
  snprintf(msg, sizeof msg, "\r\n%lu us\r\n", (unsigned long)us);
  uart_send_string(msg);
}

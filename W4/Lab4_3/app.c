/***************************************************************************//**
 * @file
 * @brief Lab 4 - Bai 3: same as Bai 2 but WITHOUT DMA (CPU sends every byte)
 *
 * PC sends one command char (read by CPU):
 *   '1' -> 10 bytes, '2' -> 100 bytes, '3' -> 1000 bytes
 * Board fills txbuf with random chars, sends it byte by byte with the CPU,
 * measures the time with TIMER0, then prints:
 *   measured time (TIMER0)  vs  theory time (W1 Bai 2 formula)
 ******************************************************************************/
#include <stdio.h>
#include "app.h"
#include "Driver/uart/uart.h"
#include "Driver/timer/timer.h"

#define TX_MAX 1000

static uint8_t  txbuf[TX_MAX];
static uint32_t seed = 12345;    // same seed as Bai 2 -> same data, fair compare

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

// CPU sends every byte itself: USART_Tx() waits for TXBL, then writes TXDATA.
// Stops at the same point as the DMA version (last byte written to TXDATA).
static uint32_t cpu_send_measure(const uint8_t *buf, uint16_t len)
{
  timer_start();
  for (uint16_t i = 0; i < len; i++) {
    uart_send_byte(buf[i]);      // CPU is busy the whole time
  }
  return timer_stop_us();
}

// W1 Bai 2 formula: time = chars * bits_per_frame / baud
static uint32_t theory_us(uint32_t chars)
{
  uint8_t  halves = uart_frame_halfbits();   // bits per frame x2 (1.5 stop bit case)
  uint32_t baud   = uart_actual_baud();
  uint64_t num    = (uint64_t)chars * halves * 1000000u;

  return (uint32_t)((num + baud) / (2u * (uint64_t)baud));   // rounded
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
  char msg[64];
  uint8_t halves;

  uart_init(115200, 8, 'N', 2, 16);   // 8N1, stop_halves = 2 -> 1 stop bit
  timer_init();

  halves = uart_frame_halfbits();
  uart_send_string("\r\nLab 4 - Bai 3: CPU Memory -> UART timing (no DMA)\r\n");
  snprintf(msg, sizeof msg, "TIMER0 %lu Hz | baud %lu | %u.%u bits/frame\r\n",
           (unsigned long)timer_clock_hz(),
           (unsigned long)uart_actual_baud(),
           halves / 2u, (halves % 2u) * 5u);
  uart_send_string(msg);
  print_help();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  char msg[64];
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

  uint32_t us = cpu_send_measure(txbuf, len);

  // Print AFTER the timer is stopped so printing is not measured
  snprintf(msg, sizeof msg, "\r\nCPU: %lu us | theory: %lu us\r\n",
           (unsigned long)us, (unsigned long)theory_us(len));
  uart_send_string(msg);
}

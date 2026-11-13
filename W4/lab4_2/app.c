/***************************************************************************//**
 * @file
 * @brief Lab 4 - CA3 Cau 3: send TX_LEN bytes Memory -> USART, DMA vs CPU
 *
 * PC sends one command char (CPU reads it):
 *   '0' -> CPU fills ALL the data, then sends it byte by byte (no DMA)
 *   '1' -> ping-pong with LDMA (channel 1), CHUNK bytes at a time:
 *          fill buf 1 -> DMA sends buf 1 while CPU fills buf 2
 *          -> DMA sends buf 2 while CPU fills buf 1 -> ... (N_CHUNKS times)
 * Timer counts fill + send for both, so the overlap shows up in the time.
 ******************************************************************************/
#include <stdio.h>
#include "app.h"
#include "Driver/uart/uart.h"
#include "Driver/dma/dma.h"
#include "Driver/timer/timer.h"

#define TX_LEN   1000                // total bytes per test
#define CHUNK    10                 // bytes per ping-pong buffer
#define N_CHUNKS (TX_LEN / CHUNK)    // 10 rounds of fill + send

static uint8_t  txbuf[TX_LEN];        // CPU method buffer
static uint8_t  pp_buf[2][CHUNK];     // DMA ping-pong: buf 1 / buf 2 (static: DMA
                                      // reads it after the call returns)
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

// CPU method: CPU fills everything, then writes every byte itself
// (USART_Tx() waits for TXBL each time). Fill and send can't overlap.
static uint32_t cpu_fill_send(void)
{
  timer_start();
  fill_data(txbuf, TX_LEN);
  for (uint16_t i = 0; i < TX_LEN; i++) {
    uart_send_byte(txbuf[i]);
  }
  return timer_stop_us();
}

// DMA method, ping-pong with two CHUNK-byte buffers, repeated N_CHUNKS times:
//   DMA sends buf[cur] while the CPU fills buf[next], then swap.
static uint32_t dma_fill_send(void)
{
  uint8_t cur = 0;

  timer_start();
  fill_data(pp_buf[0], CHUNK);                  // first chunk ready
  for (uint16_t k = 0; k < N_CHUNKS; k++) {
    dma_uart_tx_start(pp_buf[cur], CHUNK);      // DMA sends buf[cur] ...
    if (k + 1u < N_CHUNKS) {
      fill_data(pp_buf[cur ^ 1u], CHUNK);       // ... while CPU fills the other
    }
    while (!dma_uart_tx_done()) {               // wait before reusing buf[cur]
    }
    cur ^= 1u;                                  // swap
  }
  return timer_stop_us();
}

// Theory (W1 Bai 2 formula): time = chars * bits_per_frame / baud
static uint32_t theory_us(uint32_t chars)
{
  uint8_t  halves = uart_frame_halfbits();   // bits per frame x2
  uint32_t baud   = uart_actual_baud();
  uint64_t num    = (uint64_t)chars * halves * 1000000u;

  return (uint32_t)((num + baud) / (2u * (uint64_t)baud));   // rounded
}

// Print AFTER the timer is stopped so printing is not measured
static void report(const char *name, uint32_t total)
{
  char msg[96];

  snprintf(msg, sizeof msg,
           "\r\n[%s] %u B fill + send: %lu us | send (theory): %lu us\r\n",
           name, (unsigned)TX_LEN, (unsigned long)total,
           (unsigned long)theory_us(TX_LEN));
  uart_send_string(msg);
}

static void print_help(void)
{
  uart_send_string("\r\nSend 0 (CPU, no DMA) or 1 (DMA ping-pong)\r\n");
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  char msg[64];
  uint8_t halves;

  uart_init(115200, 8, 'N', 2, 16);   // 8N1, stop_halves = 2 -> 1 stop bit
  dma_init();
  timer_init();

  halves = uart_frame_halfbits();
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
  if (!uart_recv_ready()) {
    return;                      // no command yet
  }

  uint8_t cmd = uart_recv_byte(); // CPU receives and handles the command

  switch (cmd) {
    case '0':
      report("CPU", cpu_fill_send());
      break;

    case '1':
      report("DMA", dma_fill_send());
      break;

    case '\r':
    case '\n':
      break;                     // ignore Enter

    default:
      print_help();
      break;
  }
}

/***************************************************************************//**
 * @file
 * @brief Lab 4 - CA3 Cau 3: send TX_LEN bytes Memory -> USART, DMA vs CPU
 *
 * PC sends one command char (CPU reads it):
 *   '0' -> send with CPU (byte by byte, no DMA)
 *   '1' -> send with LDMA (channel 1)
 * Board replies with the data, then the time measured with TIMER0.
 * For DMA it also prints how many loop rounds the CPU had free while waiting.
 ******************************************************************************/
#include <stdio.h>
#include "app.h"
#include "Driver/uart/uart.h"
#include "Driver/dma/dma.h"
#include "Driver/timer/timer.h"

#define TX_LEN 1000   // bytes per test, must be <= DMA_MAX_XFER (2048)

static uint8_t  txbuf[TX_LEN];   // static: DMA reads it after the call returns
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

// DMA method: LDMA moves the bytes, CPU only waits for the done interrupt.
// idle = how many loop rounds the CPU was free (could do other work / sleep).
static uint32_t dma_send_measure(const uint8_t *buf, uint16_t len, uint32_t *idle)
{
  uint32_t free_rounds = 0;

  timer_start();
  dma_uart_tx_start(buf, len);
  while (!dma_uart_tx_done()) {
    free_rounds++;               // CPU is NOT needed for the transfer
  }
  uint32_t us = timer_stop_us();

  *idle = free_rounds;
  return us;
}

// CPU method: CPU writes every byte itself. USART_Tx() waits for TXBL,
// so the CPU is busy for the whole transfer (0 free rounds).
// Stops at the same point as the DMA version (last byte written to TXDATA).
static uint32_t cpu_send_measure(const uint8_t *buf, uint16_t len)
{
  timer_start();
  for (uint16_t i = 0; i < len; i++) {
    uart_send_byte(buf[i]);
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

static void run_dma(void)
{
  char msg[80];
  uint32_t idle;
  uint32_t us = dma_send_measure(txbuf, TX_LEN, &idle);

  // Print AFTER the timer is stopped so printing is not measured
  snprintf(msg, sizeof msg, "\r\n[DMA] %u B: %lu us | CPU free rounds: %lu\r\n",
           (unsigned)TX_LEN, (unsigned long)us, (unsigned long)idle);
  uart_send_string(msg);
}

static void run_cpu(void)
{
  char msg[80];
  uint32_t us = cpu_send_measure(txbuf, TX_LEN);

  snprintf(msg, sizeof msg, "\r\n[CPU] %u B: %lu us | CPU free rounds: 0\r\n",
           (unsigned)TX_LEN, (unsigned long)us);
  uart_send_string(msg);
}

static void print_theory(void)
{
  char msg[48];

  snprintf(msg, sizeof msg, "[Theory] %u B: %lu us\r\n",
           (unsigned)TX_LEN, (unsigned long)theory_us(TX_LEN));
  uart_send_string(msg);
}

static void print_help(void)
{
  uart_send_string("\r\nSend 0 (CPU, no DMA) or 1 (DMA)\r\n");
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
  uart_send_string("\r\nLab 4 - Cau 3: Memory -> USART, DMA vs CPU\r\n");
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
      fill_data(txbuf, TX_LEN);  // prepare data BEFORE timing
      run_cpu();
      print_theory();
      break;

    case '1':
      fill_data(txbuf, TX_LEN);
      run_dma();
      print_theory();
      break;

    case '\r':
    case '\n':
      break;                     // ignore Enter

    default:
      print_help();
      break;
  }
}

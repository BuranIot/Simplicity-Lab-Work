/***************************************************************************//**
 * @file
 * @brief Lab 4 - CA3 Cau 1: MOST frequent character
 *
 * PC sends a paragraph ending with Enter.
 * - USART0 RX -> inbuf is moved by LDMA (channel 0), CPU does not touch RX.
 * - CPU scans the bytes the DMA has already written, looking for '\r'/'\n'.
 * - On Enter (or buffer full): stop DMA, count letters (upper and lower case
 *   counted separately), reply "c: n" with CPU.
 *
 * Example: "aabbaAc" -> a:3 b:2 A:1 c:1 -> "a: 3"
 * Tie -> smallest ASCII wins ("abab" -> "a: 2", "Aa" -> "A: 1").
 ******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include "app.h"
#include "Driver/uart/uart.h"
#include "Driver/dma/dma.h"

#define BUFLEN 1024   // must be <= DMA_MAX_XFER

static uint8_t  inbuf[BUFLEN];
static uint16_t scanned = 0;   // how many bytes of inbuf already checked for Enter

static void rx_restart(void)
{
  scanned = 0;
  dma_uart_rx_start(inbuf, BUFLEN);
}

static void find_highest(const uint8_t *buf, uint16_t len)
{
  uint16_t count[256] = {0};   // count['a'] = how many 'a'
  uint8_t  best = 0;
  uint16_t best_cnt = 0;
  char     msg[32];

  for (uint16_t i = 0; i < len; i++) {
    count[buf[i]]++;
  }

  // Walk in ASCII order; '>' (not '>=') keeps the smallest ASCII on a tie
  for (uint16_t c = 'A'; c <= 'z'; c++) {
    if (isalpha(c) && count[c] > best_cnt) {
      best     = (uint8_t)c;
      best_cnt = count[c];
    }
  }

  if (best_cnt == 0) {
    uart_send_string("No letter\r\n");
    return;
  }

  snprintf(msg, sizeof msg, "%c: %u\r\n", best, best_cnt);
  uart_send_string(msg);
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  uart_init(115200, 8, 'N', 2, 16);   // 8N1, stop_halves = 2 -> 1 stop bit
  dma_init();

  uart_send_string("\r\nLab 4 - Cau 1: most frequent letter, send a paragraph + Enter\r\n");
  rx_restart();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  bool     full     = dma_uart_rx_done();    // read BEFORE the count
  uint16_t received = dma_uart_rx_count();

  // Advance over the new bytes until we hit Enter
  while (scanned < received && inbuf[scanned] != '\r' && inbuf[scanned] != '\n') {
    scanned++;
  }

  bool got_enter = (scanned < received);
  if (!got_enter && !full) {
    return;                      // message not finished yet
  }

  dma_uart_rx_stop();

  // "\r\n" from the terminal: the '\n' arrives as an empty message -> ignore it
  if (scanned > 0) {
    find_highest(inbuf, scanned);   // scanned = length before Enter (or BUFLEN)
  }

  rx_restart();
}

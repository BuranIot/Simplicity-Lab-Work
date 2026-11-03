/***************************************************************************//**
 * @file
 * @brief Lab 4 - Bai 1: most frequent character
 *
 * PC sends a paragraph ending with Enter.
 * - USART0 RX -> inbuf is moved by LDMA (channel 0), CPU does not touch RX.
 * - CPU scans the bytes the DMA has already written, looking for '\r'/'\n'.
 * - On Enter (or buffer full): stop DMA, count chars, reply "c: n" with CPU.
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "Driver/uart/uart.h"
#include "Driver/dma/dma.h"

#define BUFLEN 1024   // must be <= DMA_MAX_XFER

static uint8_t  inbuf[BUFLEN];
static uint16_t scanned = 0;     // how many bytes of inbuf already checked for Enter
static uint16_t count[256];      // count['a'] = how many 'a'

static void rx_restart(void)
{
  scanned = 0;
  dma_uart_rx_start(inbuf, BUFLEN);
}

// Option A: count everything first, then one pass over 256 slots to find max.
// Tie -> smallest ASCII wins.
static void find_most_frequent(const uint8_t *buf, uint16_t len)
{
  char msg[32];
  uint16_t maxCount = 0;
  uint8_t  maxChar  = 0;

  memset(count, 0, sizeof count);

  for (uint16_t i = 0; i < len; i++) {
    count[buf[i]]++;             // buf is uint8_t -> index always 0..255
  }

  // Only printable, non-space chars ('!'..'~') take part
  for (uint16_t c = '!'; c <= '~'; c++) {
    if (count[c] > maxCount) {
      maxCount = count[c];
      maxChar  = (uint8_t)c;
    }
  }

  if (maxCount == 0) {
    uart_send_string("No character\r\n");
    return;
  }

  snprintf(msg, sizeof msg, "%c: %u\r\n", maxChar, maxCount);
  uart_send_string(msg);         // short reply -> CPU fills TX buffer
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  uart_init(115200, 8, 'N', 2, 16);   // 8N1, stop_halves = 2 -> 1 stop bit
  dma_init();

  uart_send_string("\r\nLab 4 - Bai 1: send a paragraph, end with Enter\r\n");
  rx_restart();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  uint16_t received = dma_uart_rx_count();
  int16_t  end = -1;

  // Look only at the new bytes the DMA wrote since last time
  while (scanned < received) {
    if (inbuf[scanned] == '\r' || inbuf[scanned] == '\n') {
      end = (int16_t)scanned;
      break;
    }
    scanned++;
  }

  if (end < 0 && dma_uart_rx_done()) {
    end = BUFLEN;                // buffer full without Enter -> use all of it
  }

  if (end < 0) {
    return;                      // message not finished yet
  }

  dma_uart_rx_stop();

  // "\r\n" from the terminal: the '\n' arrives as an empty message -> ignore it
  if (end > 0) {
    find_most_frequent(inbuf, (uint16_t)end);
  }

  rx_restart();
}

/***************************************************************************//**
 * @file
 * @brief Lab 4 - CA3 Cau 2: count "aba" (overlaps included) in 16 bytes
 *
 * PC sends a 16-byte string.
 * - LDMA (channel 0) moves the bytes from USART0 RX buffer into inbuf (RAM).
 * - CPU searches inbuf for PATTERN, overlapping matches count too.
 * - CPU writes the result into the USART TX buffer: "aba - 4 lan".
 *
 * Example: abacbabacxyababa -> matches at 0, 5, 11, 13 -> "aba - 4 lan"
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "Driver/uart/uart.h"
#include "Driver/dma/dma.h"

#define MSG_LEN 16                   // bytes per message from the PC
#define PATTERN "aba"
#define PAT_LEN (sizeof(PATTERN) - 1) // 3, without the '\0'

static uint8_t  inbuf[MSG_LEN];
static uint16_t got = 0;             // real (non-Enter) bytes already in inbuf

// Ask the LDMA for the bytes still missing, written right after the ones we have
static void rx_continue(void)
{
  dma_uart_rx_start(&inbuf[got], (uint16_t)(MSG_LEN - got));
}

// Remove '\r' / '\n' that DMA just wrote into inbuf[got..MSG_LEN-1].
// If the user presses Enter, those bytes must not count as data,
// otherwise every next message is shifted.
static void drop_enter(void)
{
  uint16_t w = got;

  for (uint16_t r = got; r < MSG_LEN; r++) {
    if (inbuf[r] != '\r' && inbuf[r] != '\n') {
      inbuf[w++] = inbuf[r];
    }
  }
  got = w;
}

// Slide a window of PAT_LEN bytes ONE step at a time -> overlaps are counted.
// e.g. "ababa": match at 0 and at 2 -> 2
static uint16_t count_pattern(const uint8_t *buf, uint16_t len)
{
  uint16_t n = 0;

  for (uint16_t i = 0; i + PAT_LEN <= len; i++) {
    if (memcmp(&buf[i], PATTERN, PAT_LEN) == 0) {
      n++;
    }
  }
  return n;
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  uart_init(115200, 8, 'N', 2, 16);   // 8N1, stop_halves = 2 -> 1 stop bit
  dma_init();

  uart_send_string("\r\nLab 4 - Cau 2: send 16 bytes, count \"" PATTERN "\"\r\n");
  got = 0;
  rx_continue();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  char msg[48];

  if (!dma_uart_rx_done()) {
    return;                          // LDMA still filling inbuf
  }

  drop_enter();                      // throw away Enter keys, keep real data

  if (got < MSG_LEN) {
    rx_continue();                   // Enter took some slots -> get the rest
    return;
  }

  // Full 16 bytes: CPU does the search
  uint16_t n = count_pattern(inbuf, MSG_LEN);

  // Reply with CPU (small amount of data)
  uart_send_string("\r\nData: ");
  for (uint16_t i = 0; i < MSG_LEN; i++) {
    uart_send_byte(inbuf[i]);
  }
  snprintf(msg, sizeof msg, "\r\n" PATTERN " - %u lan\r\n", n);
  uart_send_string(msg);

  got = 0;                           // next message
  rx_continue();
}

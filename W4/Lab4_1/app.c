/***************************************************************************//**
 * @file
 * @brief Lab 4 - CA3 Cau 1: SECOND-least frequent character
 *
 * PC sends a paragraph ending with Enter.
 * - USART0 RX -> inbuf is moved by LDMA (channel 0), CPU does not touch RX.
 * - CPU scans the bytes the DMA has already written, looking for '\r'/'\n'.
 * - On Enter (or buffer full): stop DMA, count letters (upper and lower case
 *   counted separately), bubble sort, reply "c: n" with CPU.
 *
 * Example: "aabbaAc" -> A:1 c:1 b:2 a:3 -> least = 1, second least = "b: 2"
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "Driver/uart/uart.h"
#include "Driver/dma/dma.h"

#define BUFLEN    1024  // must be <= DMA_MAX_XFER
#define NLETTERS  52    // 'A'..'Z' + 'a'..'z'
#define NTH_LEAST 2     // 1 = least, 2 = second least, ...

typedef struct {
  uint8_t  ch;    // the letter
  uint16_t cnt;   // how many times it appears
} freq_t;

static uint8_t  inbuf[BUFLEN];
static uint16_t scanned = 0;     // how many bytes of inbuf already checked for Enter
static uint16_t count[256];      // count['a'] = how many 'a'
static freq_t   f[NLETTERS];

static void rx_restart(void)
{
  scanned = 0;
  dma_uart_rx_start(inbuf, BUFLEN);
}

// Highest -> lowest by cnt. Swap only when strictly smaller -> stable,
// so equal counts keep ASCII order (smallest ASCII first inside a tie).
static void bubble_sort(freq_t *a, uint16_t n)
{
  for (uint16_t i = 0; i < n - 1; i++) {
    bool swapped = false;
    for (uint16_t j = 0; j < n - 1 - i; j++) {
      if (a[j].cnt < a[j + 1].cnt) {
        freq_t tmp = a[j];       // swap the whole pair (letter + count)
        a[j]       = a[j + 1];
        a[j + 1]   = tmp;
        swapped    = true;
      }
    }
    if (!swapped) {
      break;                     // already sorted
    }
  }
}

static void find_second_least(const uint8_t *buf, uint16_t len)
{
  char msg[48];
  uint16_t n = 0;
  int16_t  i;

  memset(count, 0, sizeof count);
  for (i = 0; i < len; i++) {
    count[buf[i]]++;             // buf is uint8_t -> index always 0..255
  }

  // Letters only, in ASCII order: 'A'..'Z' then 'a'..'z'
  for (uint16_t c = 'A'; c <= 'Z'; c++) {
    f[n].ch = (uint8_t)c; f[n].cnt = count[c]; n++;
  }
  for (uint16_t c = 'a'; c <= 'z'; c++) {
    f[n].ch = (uint8_t)c; f[n].cnt = count[c]; n++;
  }

  bubble_sort(f, n);             // high -> low, letters with cnt 0 end up last

  // Walk back from the end, skipping letters that never appear
  i = (int16_t)n - 1;
  while (i >= 0 && f[i].cnt == 0) {
    i--;
  }
  if (i < 0) {
    uart_send_string("No letter\r\n");
    return;
  }

  // f[i] is now the LEAST. Climb up one count level at a time.
  // A level = all letters with the same cnt (ties), e.g. A:1 c:1 is ONE level.
  for (uint8_t level = 1; ; level++) {
    uint16_t cur   = f[i].cnt;
    int16_t  start = i;
    while (start > 0 && f[start - 1].cnt == cur) {
      start--;                   // go to the first letter of this tie group
    }

    if (level == NTH_LEAST) {
      // f[start] = smallest ASCII inside the tie group
      snprintf(msg, sizeof msg, "%c: %u\r\n", f[start].ch, cur);
      uart_send_string(msg);     // short reply -> CPU fills TX buffer
      return;
    }

    i = start - 1;               // next (bigger) count level
    if (i < 0) {
      snprintf(msg, sizeof msg, "No level %u (only %u count levels)\r\n",
               (unsigned)NTH_LEAST, (unsigned)level);
      uart_send_string(msg);
      return;
    }
  }
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  uart_init(115200, 8, 'N', 2, 16);   // 8N1, stop_halves = 2 -> 1 stop bit
  dma_init();

  uart_send_string("\r\nLab 4 - Cau 1: 2nd least letter, send a paragraph + Enter\r\n");
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
    find_second_least(inbuf, (uint16_t)end);
  }

  rx_restart();
}

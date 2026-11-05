#ifndef DMA_H
#define DMA_H
#include <stdint.h>
#include <stdbool.h>

#define RX_LDMA_CHANNEL 0   // USART0 RXDATA -> memory
#define TX_LDMA_CHANNEL 1   // memory -> USART0 TXDATA

// One LDMA descriptor can move at most 2048 items (XFERCNT is 11 bits)
#define DMA_MAX_XFER    2048

void     dma_init(void);

// RX: USART0 -> buf, at most len bytes (len <= DMA_MAX_XFER)
void     dma_uart_rx_start(uint8_t *buf, uint16_t len);
uint16_t dma_uart_rx_count(void);   // bytes already written into buf
bool     dma_uart_rx_done(void);    // true when buf is completely full
void     dma_uart_rx_stop(void);

// TX: buf -> USART0, len bytes (len <= DMA_MAX_XFER)
void     dma_uart_tx_start(const uint8_t *buf, uint16_t len);
bool     dma_uart_tx_done(void);

#endif

#include "dma.h"
#include "em_ldma.h"
#include "em_usart.h"

static LDMA_Descriptor_t ldmaRXDescriptor;
static LDMA_TransferCfg_t ldmaRXConfig;
static LDMA_Descriptor_t ldmaTXDescriptor;
static LDMA_TransferCfg_t ldmaTXConfig;

static volatile bool rx_done = false;
static volatile bool tx_done = true;
static uint16_t rx_len = 0;

void LDMA_IRQHandler(void)
{
  uint32_t flags = LDMA_IntGet();

  if (flags & (1 << TX_LDMA_CHANNEL)) {
    LDMA_IntClear(1 << TX_LDMA_CHANNEL);
    tx_done = true;
  }

  if (flags & (1 << RX_LDMA_CHANNEL)) {
    LDMA_IntClear(1 << RX_LDMA_CHANNEL);
    rx_done = true;
  }
}

void dma_init(void)
{
  LDMA_Init_t ldmaInit = LDMA_INIT_DEFAULT;
  LDMA_Init(&ldmaInit);   // also enables LDMA_IRQn in the NVIC

  // Request signals: RXDATAV = "USART has a new byte", TXBL = "TX buffer has room"
  ldmaRXConfig = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_RXDATAV);
  ldmaTXConfig = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_TXBL);
}

void dma_uart_rx_start(uint8_t *buf, uint16_t len)
{
  if (len == 0 || len > DMA_MAX_XFER) {
    return;
  }

  rx_len  = len;
  rx_done = false;

  // Source: USART0->RXDATA (fixed), destination: buf (++ each byte)
  ldmaRXDescriptor = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_SINGLE_P2M_BYTE(&(USART0->RXDATA), buf, len);
  ldmaRXDescriptor.xfer.blockSize = ldmaCtrlBlockSizeUnit1;

  LDMA_StartTransfer(RX_LDMA_CHANNEL, &ldmaRXConfig, &ldmaRXDescriptor);
}

uint16_t dma_uart_rx_count(void)
{
  if (rx_done) {
    return rx_len;
  }
  // XFERCNT counts down as the LDMA writes bytes into buf
  return (uint16_t)(rx_len - LDMA_TransferRemainingCount(RX_LDMA_CHANNEL));
}

bool dma_uart_rx_done(void)
{
  return rx_done;
}

void dma_uart_rx_stop(void)
{
  LDMA_StopTransfer(RX_LDMA_CHANNEL);
}

void dma_uart_tx_start(const uint8_t *buf, uint16_t len)
{
  if (len == 0 || len > DMA_MAX_XFER) {
    return;
  }

  tx_done = false;

  // Source: buf (++ each byte), destination: USART0->TXDATA (fixed)
  ldmaTXDescriptor = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(buf, &(USART0->TXDATA), len);
  ldmaTXDescriptor.xfer.blockSize = ldmaCtrlBlockSizeUnit1;

  LDMA_StartTransfer(TX_LDMA_CHANNEL, &ldmaTXConfig, &ldmaTXDescriptor);
}

bool dma_uart_tx_done(void)
{
  return tx_done;
}

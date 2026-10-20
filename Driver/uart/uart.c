#include "uart.h"
#include "em_gpio.h"
#include "em_usart.h"

#define BSP_TXPORT gpioPortA
#define BSP_RXPORT gpioPortA
#define BSP_TXPIN 5
#define BSP_RXPIN 6
#define BSP_ENABLE_PORT gpioPortD
#define BSP_ENABLE_PIN 4

static uint8_t uart_databits;
static char    uart_parity;
static uint8_t uart_stop_halves;

static void initGPIO(void)
{
  // Configure the USART TX pin to the board controller as an output
  GPIO_PinModeSet(BSP_TXPORT, BSP_TXPIN, gpioModePushPull, 1);

  // Configure the USART RX pin to the board controller as an input
  GPIO_PinModeSet(BSP_RXPORT, BSP_RXPIN, gpioModeInput, 0);

  GPIO_PinModeSet(BSP_ENABLE_PORT, BSP_ENABLE_PIN, gpioModePushPull, 1);
}

static void initUSART0(uint32_t baud, uint8_t databits, char parity, uint8_t stop_halves, uint8_t ovs)
{
  // Default asynchronous initializer (115.2 Kbps, 8N1, no flow control)
  //USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
  USART_InitAsync_TypeDef init;

  init.enable = usartEnable;
  init.refFreq = 0;
  init.baudrate = baud;

  switch (ovs) {
    case 8:  init.oversampling = usartOVS8; break;
    case 6:  init.oversampling = usartOVS6; break;
    case 4:  init.oversampling = usartOVS4; break;
    default: init.oversampling = usartOVS16; break;
  }

  switch (databits) {
    case 5:  init.databits = usartDatabits5; break;
    case 6:  init.databits = usartDatabits6; break;
    case 7:  init.databits = usartDatabits7; break;
    default: init.databits = usartDatabits8; break;
  }

  switch (parity) {
    case 'E': init.parity = usartEvenParity; break;
    case 'O': init.parity = usartOddParity; break;
    default:  init.parity = usartNoParity; break;
  }

  switch (stop_halves) {
    case 1:  init.stopbits = usartStopbits0p5; break;
    case 3:  init.stopbits = usartStopbits1p5; break;
    case 4:  init.stopbits = usartStopbits2; break;
    default: init.stopbits = usartStopbits1; break;
  }

  init.mvdis = false;
  init.prsRxEnable = false;
  init.prsRxCh = 0;

  init.autoCsEnable = false;
  init.csInv = false;
  init.autoCsHold = 0;
  init.autoCsSetup = 0;
  init.hwFlowControl = usartHwFlowControlNone;

  // Route USART0 TX and RX to the board controller TX and RX pins
  GPIO->USARTROUTE[0].TXROUTE = (BSP_TXPORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
            | (BSP_TXPIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].RXROUTE = (BSP_RXPORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
            | (BSP_RXPIN << _GPIO_USART_RXROUTE_PIN_SHIFT);

  // Enable RX and TX signals now that they have been routed
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;

  // Configure and enable USART0
  USART_InitAsync(USART0, &init);
}

uint8_t uart_frame_halfbits(void){
  uint8_t parity_bit = (uart_parity == 'E' || uart_parity == '0') ? 1:0;

  return(uint8_t)(2*(1 + uart_databits + parity_bit) + uart_stop_halves);
}

uint32_t uart_actual_baud(void){
  return USART_BaudrateGet(USART0);
}

void uart_init(uint32_t baud, uint8_t databits, char parity,
               uint8_t stop_halves, uint8_t ovs)
{
  uart_databits = databits;
  uart_parity = parity;
  uart_stop_halves = stop_halves;

  initGPIO();
  initUSART0(baud, databits, parity, stop_halves, ovs);
}
void    uart_send_byte(uint8_t b){
    USART_Tx(USART0, b);
}
uint8_t uart_recv_byte(void){
    return USART_Rx(USART0);
}

void uart_send_string(const char *s)
{
  while (*s != '\0') {      
    uart_send_byte(*s);     
    s++;                   
  }
}

int uart_recv_line(char *buf, int max){
  int count = 0;

  while(count < max - 1){
    char c = (char)uart_recv_byte();

    if(c == '\n' || c == '\r'){
      break;
    }
    buf[count] = c;
    count++;
  }

  buf[count] = '\0';
  return count;
}


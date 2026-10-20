#include "uart.h"
#include "em_gpio.h"
#include "em_usart.h"

#define BSP_TXPORT gpioPortA
#define BSP_RXPORT gpioPortA
#define BSP_TXPIN 5
#define BSP_RXPIN 6
#define BSP_ENABLE_PORT gpioPortD
#define BSP_ENABLE_PIN 4

static void initGPIO(void)
{
  // Configure the USART TX pin to the board controller as an output
  GPIO_PinModeSet(BSP_TXPORT, BSP_TXPIN, gpioModePushPull, 1);

  // Configure the USART RX pin to the board controller as an input
  GPIO_PinModeSet(BSP_RXPORT, BSP_RXPIN, gpioModeInput, 0);

  /*
   * Configure the BCC_ENABLE pin as output and set high.  This enables
   * the virtual COM port (VCOM) connection to the board controller and
   * permits serial port traffic over the debug connection to the host
   * PC.
   *
   * To disable the VCOM connection and use the pins on the kit
   * expansion (EXP) header, comment out the following line.
   */
  GPIO_PinModeSet(BSP_ENABLE_PORT, BSP_ENABLE_PIN, gpioModePushPull, 1);
}

static void initUSART0(void)
{
  // Default asynchronous initializer (115.2 Kbps, 8N1, no flow control)
  //USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
  USART_InitAsync_TypeDef init;

  init.enable = usartEnable;
  init.refFreq = 0;
  init.baudrate = 115200;
  init.oversampling = usartOVS16;
  init.databits = usartDatabits8;
  init.parity = usartNoParity;
  init.stopbits = usartStopbits1;

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

void    uart_init(void){
  initGPIO();
  initUSART0();    
}
void    uart_send_byte(uint8_t b){
    USART_Tx(USART0, b);
}
uint8_t uart_recv_byte(void){
    return USART_Rx(USART0);
}

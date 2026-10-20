#ifndef UART_H
#define UART_H
#include <stdint.h>

void    uart_init(void);
void    uart_send_byte(uint8_t b);
uint8_t uart_recv_byte(void);

#endif

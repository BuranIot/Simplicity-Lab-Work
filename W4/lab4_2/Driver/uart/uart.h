#ifndef UART_H
#define UART_H
#include <stdint.h>
#include <stdbool.h>

void    uart_init(uint32_t baud, uint8_t databits, char parity, uint8_t stop_halves, uint8_t ovs);
void    uart_send_byte(uint8_t b);
void    uart_send_string(const char *s);
uint8_t uart_recv_byte(void);
bool    uart_recv_ready(void);   // true if a byte is waiting (non-blocking)
int     uart_recv_line(char *buf, int max);
uint8_t uart_frame_halfbits(void);
uint32_t uart_actual_baud(void);

#endif

#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>

// TIMER0 used as a free-running 32-bit stopwatch
void     timer_init(void);
void     timer_start(void);      // reset counter to 0 and start counting
uint32_t timer_stop_us(void);    // stop and return elapsed time in microseconds
uint32_t timer_clock_hz(void);   // TIMER0 input clock, for the report

#endif

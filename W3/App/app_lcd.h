/*
 * app_lcd.h
 *
 *  Lab 3 SPI - clock application on the memory LCD.
 */

#ifndef APP_LCD_H_
#define APP_LCD_H_

#include <stdint.h>

typedef struct time
{
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
} time_lcd_t;

void memlcd_app_init(void);

void memlcd_app_process_action(void);

#endif /* APP_LCD_H_ */

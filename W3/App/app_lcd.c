/*
 * lcd.c
 *
 *  Created on: Nov 4, 2024
 *      Author: Phat_Dang
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "sl_board_control.h"
#include "sl_sleeptimer.h"
#include "em_assert.h"
#include "Middleware/glib/glib.h"
#include "Middleware/glib/dmd.h"

#ifndef LCD_MAX_LINES
#define LCD_MAX_LINES      11
#endif

/* Clock tick period */
#define CLOCK_TICK_MS      1000

/* Position of the big HH:MM:SS text (16x20 font, 8 chars = 128 px wide) */
#define CLOCK_X            0
#define CLOCK_Y            54

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static GLIB_Context_t glibContext;
static int currentLine = 0;

static sl_sleeptimer_timer_handle_t clock_timer;

/* Seconds since midnight, counted in the timer callback */
static volatile uint32_t clock_seconds;

/* Set by the timer callback, cleared after the display is redrawn */
static volatile bool clock_update = false;

/*******************************************************************************
 ***************************  LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/***************************************************************************//**
 * Sleeptimer callback, runs every CLOCK_TICK_MS in interrupt context.
 * Only counts and sets a flag; SPI drawing is done in the main loop.
 ******************************************************************************/
static void clock_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;

  clock_seconds = (clock_seconds + 1) % (24 * 3600);
  clock_update = true;
}

/***************************************************************************//**
 * Draw the current time as HH:MM:SS and send it to the LCD.
 ******************************************************************************/
static void clock_draw(void)
{
  char buf[9];
  uint32_t t = clock_seconds;

  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
           (unsigned long)(t / 3600),
           (unsigned long)((t / 60) % 60),
           (unsigned long)(t % 60));

  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNumber16x20);
  GLIB_drawString(&glibContext, buf, strlen(buf), CLOCK_X, CLOCK_Y, true);

  DMD_updateDisplay();
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize example.
 ******************************************************************************/
void memlcd_app_init(void)
{
  uint32_t status;

  /* Enable the memory lcd */
  status = sl_board_enable_display();
  EFM_ASSERT(status == SL_STATUS_OK);

  /* Initialize the DMD support for memory lcd display */
  status = DMD_init(0);
  EFM_ASSERT(status == DMD_OK);

  /* Initialize the glib context */
  status = GLIB_contextInit(&glibContext);
  EFM_ASSERT(status == GLIB_OK);

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  /* Fill lcd with background color */
  GLIB_clear(&glibContext);

  /* Use Narrow font */
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNarrow6x8);

  /* Draw text on the memory lcd display*/
  GLIB_drawStringOnLine(&glibContext,
                        "LAB 3 SPI",
                        currentLine++,
                        GLIB_ALIGN_LEFT,
                        5,
                        5,
                        true);

  GLIB_drawStringOnLine(&glibContext,
                        "CLOCK",
                        currentLine++,
                        GLIB_ALIGN_LEFT,
                        5,
                        5,
                        true);

  /* Start from the build time ("HH:MM:SS") */
  const char *build_time = __TIME__;
  clock_seconds = (uint32_t)((build_time[0] - '0') * 10 + (build_time[1] - '0')) * 3600
                  + (uint32_t)((build_time[3] - '0') * 10 + (build_time[4] - '0')) * 60
                  + (uint32_t)((build_time[6] - '0') * 10 + (build_time[7] - '0'));

  clock_draw();

  /* Tick every second */
  status = sl_sleeptimer_start_periodic_timer_ms(&clock_timer,
                                                 CLOCK_TICK_MS,
                                                 clock_timer_callback,
                                                 NULL,
                                                 0,
                                                 0);
  EFM_ASSERT(status == SL_STATUS_OK);
}


/***************************************************************************//**
 * Ticking function.
 ******************************************************************************/
void memlcd_app_process_action(void)
{
  if (clock_update) {
    clock_update = false;
    clock_draw();
  }
}


/*
 * app_lcd.c
 *
 *  Lab 3 SPI - clock application on the memory LCD.
 *  Shows student name, student ID and HH:MM:SS (starting at 00:00:00).
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "sl_board_control.h"
#include "sl_sleeptimer.h"
#include "em_assert.h"
#include "em_core.h"
#include "Middleware/glib/glib.h"
#include "Middleware/glib/dmd.h"
#include "App/app_lcd.h"

/* Student info shown on the LCD (narrow font: max 21 chars per line) */
#define STUDENT_NAME       "Tran Dong Duong"
#define STUDENT_ID         "MSSV: 23207049"

/* Clock tick period */
#define CLOCK_TICK_MS      1000

/* Name / ID lines (6x8 font), centered */
#define TEXT_LINE_NAME     0
#define TEXT_LINE_ID       1
#define TEXT_Y_OFFSET      36

/* Position of the big HH:MM:SS text (16x20 font, 8 chars = 128 px wide) */
#define CLOCK_X            0
#define CLOCK_Y            60

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static GLIB_Context_t glibContext;

static sl_sleeptimer_timer_handle_t clock_timer;

/* Current time, advanced in the timer callback */
static volatile time_lcd_t clock_time = { 0, 0, 0 };

/* Set by the timer callback, cleared after the display is redrawn */
static volatile bool clock_update = false;

/*******************************************************************************
 ***************************  LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/***************************************************************************//**
 * Advance the time by one second, wrapping at 23:59:59.
 ******************************************************************************/
static void time_lcd_tick(volatile time_lcd_t *t)
{
  if (++t->seconds >= 60) {
    t->seconds = 0;
    if (++t->minutes >= 60) {
      t->minutes = 0;
      if (++t->hours >= 24) {
        t->hours = 0;
      }
    }
  }
}

/***************************************************************************//**
 * Sleeptimer callback, runs every CLOCK_TICK_MS in interrupt context.
 * Only counts and sets a flag; SPI drawing is done in the main loop.
 ******************************************************************************/
static void clock_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;

  time_lcd_tick(&clock_time);
  clock_update = true;
}

/***************************************************************************//**
 * Draw the current time as HH:MM:SS and send it to the LCD.
 ******************************************************************************/
static void clock_draw(void)
{
  char buf[9];
  time_lcd_t t;
  EMSTATUS status;

  /* Copy the time atomically so the callback can't change it half-way */
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_ATOMIC();
  t.hours   = clock_time.hours;
  t.minutes = clock_time.minutes;
  t.seconds = clock_time.seconds;
  CORE_EXIT_ATOMIC();

  snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
           (unsigned)t.hours, (unsigned)t.minutes, (unsigned)t.seconds);

  status = GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNumber16x20);
  EFM_ASSERT(status == GLIB_OK);
  status = GLIB_drawString(&glibContext, buf, strlen(buf), CLOCK_X, CLOCK_Y, true);
  EFM_ASSERT(status == GLIB_OK);

  status = DMD_updateDisplay();
  EFM_ASSERT(status == DMD_OK);
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize the LCD, draw name / ID / 00:00:00 and start the 1 s timer.
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
  status = GLIB_clear(&glibContext);
  EFM_ASSERT(status == GLIB_OK);

  /* Use Narrow font so longer names still fit on one line */
  status = GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNarrow6x8);
  EFM_ASSERT(status == GLIB_OK);

  /* Student name and ID, centered */
  status = GLIB_drawStringOnLine(&glibContext,
                                 STUDENT_NAME,
                                 TEXT_LINE_NAME,
                                 GLIB_ALIGN_CENTER,
                                 0,
                                 TEXT_Y_OFFSET,
                                 true);
  EFM_ASSERT(status == GLIB_OK);

  status = GLIB_drawStringOnLine(&glibContext,
                                 STUDENT_ID,
                                 TEXT_LINE_ID,
                                 GLIB_ALIGN_CENTER,
                                 0,
                                 TEXT_Y_OFFSET,
                                 true);
  EFM_ASSERT(status == GLIB_OK);

  /* Start from 00:00:00 */
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
 * Ticking function: redraw the clock when the timer says a second passed.
 ******************************************************************************/
void memlcd_app_process_action(void)
{
  if (clock_update) {
    clock_update = false;
    clock_draw();
  }
}

#include "timer.h"
#include "em_timer.h"
#include "em_cmu.h"
#include "sl_clock_manager.h"

static uint32_t timer_freq = 0;

void timer_init(void)
{
  sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_TIMER0);

  // TIMER0 input clock (EM01GRPACLK on xG21), same call as the lab PDF
  timer_freq = CMU_ClockFreqGet(cmuClock_TIMER0);

  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.enable = false;           // don't start counting yet
  TIMER_Init(TIMER0, &timerInit);
  TIMER_TopSet(TIMER0, 0xFFFFFFFF);   // full 32-bit range, no early wrap
}

void timer_start(void)
{
  TIMER_CounterSet(TIMER0, 0);
  TIMER_Enable(TIMER0, true);
}

uint32_t timer_stop_us(void)
{
  TIMER_Enable(TIMER0, false);
  uint32_t numClk = TIMER_CounterGet(TIMER0);

  if (timer_freq == 0) {
    return 0;
  }
  // 64-bit math: exact even when the clock is not a whole number of MHz (e.g. 38.4 MHz)
  return (uint32_t)(((uint64_t)numClk * 1000000ULL) / timer_freq);
}

uint32_t timer_clock_hz(void)
{
  return timer_freq;
}

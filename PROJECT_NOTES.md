# GTMT Lab Notes (HK1 2026-27)

## Setup

- **Board:** BRD4180B, MCU **EFR32MG21A020F1024IM32**
- **IDE:** Simplicity Studio **v5**. v6 builds differently, so I went back to v5.
- **SDK:** simplicity_sdk_2025.6.3, toolchain GNU ARM v12.2.1
- **Workflow:** I write the code myself; Claude only reviews and fixes it (no full solutions).

## Folder layout

| Path | What |
|---|---|
| `PDF/` | Lab handouts (Lab_1, 3, 4, 5, 6) |
| `SourceCode/` | Teacher's reference code: `UART`, `SPI`, `I2C`, `DMA`, `ADV`, `MESH` |
| `W1/BT1/BT1/` | Lab 1, exercise 1: UART + LED |
| `W1/BT2/` | Lab 1, exercise 2: UART line in, frame info out |
| `W1/PC_BT2/uart_pc_bt2.c` | PC-side (Windows) serial program for BT2 |

## Reusable modules (copy into new projects)

### `uart/uart.c` + `uart/uart.h` (latest version is in `W1/BT2/uart/`)

```c
void     uart_init(uint32_t baud, uint8_t databits, char parity, uint8_t stop_halves, uint8_t ovs);
void     uart_send_byte(uint8_t b);
void     uart_send_string(const char *s);
uint8_t  uart_recv_byte(void);
int      uart_recv_line(char *buf, int max);
uint8_t  uart_frame_halfbits(void);   // frame length in half-bits
uint32_t uart_actual_baud(void);
```

- Uses USART0: TX = **PA5**, RX = **PA6**, VCOM enable = **PD4**
- `stop_halves` counts half stop bits: 2 = 1 stop bit, 3 = 1.5, 4 = 2
- Typical call: `uart_init(115200, 8, 'N', 2, 16);`

### `led/led.c` + `led/led.h` (in `W1/BT1/BT1/led/`)

```c
void led_init(void);
void led_on(uint8_t led);      // led = 0 or 1
void led_off(uint8_t led);
void led_toggle(uint8_t led);
```

## Project conventions

- Put user code in `app.c` (`app_init()` and `app_process_action()`), not in `main.c`
- Keep drivers in their own folders (`uart/`, `led/`) and include them as `#include "uart/uart.h"`
- `em_uart` in the SDK is not what the lab wants, so I write my own USART0 driver with `em_usart` / `em_cmu` / `em_gpio`

## PC side

- `uart_pc_bt2.c` is a Windows program: set `COM_PORT` to the "JLink CDC UART Port (COMx)" shown in Device Manager
- Its settings must match the board: 115200, 8 data bits, no parity, 1 stop bit
- Any serial terminal at the same settings also works for testing

## Gotchas I hit

- Typo `uint8_d` instead of `uint8_t`
- `snprintf` needs `#include <stdio.h>`
- `CMU_ClockFreqGet` / `cmuClock_PCLK` need `#include "em_cmu.h"`
- Declare a static helper before using it, or you get "implicit declaration"
- Board not showing up for flashing: check the USB cable and the J-Link connection in Simplicity Studio's Debug Adapters view

## Status

- [x] Lab 1, BT1: UART echo + LED
- [x] Lab 1, BT2: prints line, char count, bits/frame, baud, PCLK, transfer time (verified on board)
- [ ] Lab 3 onward

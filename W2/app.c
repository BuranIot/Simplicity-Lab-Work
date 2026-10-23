/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "sl_sleeptimer.h"
#include "Driver/uart/uart.h"
#include "Driver/i2c/i2c.h"
#include "Driver/mpu6050/mpu6050.h"

static bool sensor_ready = false;

static void print_fixed(const char *label, int32_t value, int32_t div,
                        int decimals, const char *unit){
    char send[64];
    const char *sign = (value < 0) ? "-" : "";
    if (value < 0) value = -value;

    snprintf(send, sizeof(send), "%s: %s%ld.%0*ld %s\r\n",
             label, sign,
             (long)(value / div), decimals, (long)(value % div),
             unit);
    uart_send_string(send);
}
/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
    uart_init(115200, 8, 'N', 2, 16);
    i2c_init();
    uart_send_string("\033[2J\033[H");      // cursor home
    sensor_ready = mpu6050_init();
    uart_send_string(sensor_ready ? "MPU6050 ready\r\n"
                                  : "MPU6050 init FAILED\r\n");
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  int32_t ax, ay, az, gx, gy, t;
  uart_send_string("\033[H");      // cursor home
  if (!sensor_ready){
    // retry every second until the sensor answers
    sl_sleeptimer_delay_millisecond(1000);
    sensor_ready = mpu6050_init();
    uart_send_string(sensor_ready ? "MPU6050 ready\r\n"
                                      : "MPU6050 init FAILED\r\n");
    return;
  } 

  if (mpu6050_read_accel(&ax, &ay, &az)){
    print_fixed("AccelX", ax, 1000, 3, "g");
    print_fixed("AccelY", ay, 1000, 3, "g");
    print_fixed("AccelZ", az, 1000, 3, "g");
  } else {
  uart_send_string("accel read FAIL\r\n");
}

  if (mpu6050_read_gyro(&gx, &gy)){
      print_fixed("GyroX", gx, 1000, 2 + 1, "deg/s");
      print_fixed("GyroY", gy, 1000, 2 + 1, "deg/s");
  } else {
  uart_send_string("gyro read FAIL\r\n");
}

  if (mpu6050_read_temp(&t)){
      print_fixed("Temperature", t, 100, 2, "C");
  } else {
  uart_send_string("temp read FAIL\r\n");
}

  uart_send_string("\r\n");
  sl_sleeptimer_delay_millisecond(250);
}

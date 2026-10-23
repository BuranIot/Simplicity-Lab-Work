#include "mpu6050.h"
#include "../i2c/i2c.h"
#include "sl_sleeptimer.h"

#define REG_SMPLRT_DIV   0x19   // Sample rate divider: rate = gyro output rate / (1 + value)
#define REG_CONFIG       0x1A   // DLPF_CFG [2:0]: low-pass filter, 0/7 -> gyro 8 kHz, 1..6 -> 1 kHz
#define REG_GYRO_CONFIG  0x1B   // FS_SEL [4:3]: gyro range 0=+-250, 1=+-500, 2=+-1000, 3=+-2000 deg/s
#define REG_ACCEL_CONFIG 0x1C   // AFS_SEL [4:3]: accel range 0=+-2g, 1=+-4g, 2=+-8g, 3=+-16g
#define REG_ACCEL_XOUT_H 0x3B   // First accel output byte, 6 bytes: X_H X_L Y_H Y_L Z_H Z_L
#define REG_TEMP_OUT_H   0x41   // Temperature output, 2 bytes: TEMP_H TEMP_L
#define REG_GYRO_XOUT_H  0x43   // First gyro output byte, 6 bytes: X_H X_L Y_H Y_L Z_H Z_L
#define REG_PWR_MGMT_1   0x6B   // Power: DEVICE_RESET bit 7, SLEEP bit 6, CLKSEL [2:0] (0 = internal 8 MHz)
#define REG_WHO_AM_I     0x75   // Device ID, reads 0x68 on a working MPU6050

#define ACCEL_LSB_PER_G   8192   // Accel sensitivity in LSB per g, depends on AFS_SEL (0 -> 8192, +-4g)
#define GYRO_LSB_PER_DPS  655     // Gyro sensitivity in LSB per deg/s, depends on FS_SEL (0 -> 655, +-500 deg/s)

static bool write_reg(
  uint8_t reg, 
  uint8_t value){

  return i2c_write_register(MPU6050_ADDR, reg, value) ==
  i2cTransferDone;
}

static bool read_regs(
  uint8_t reg, 
  uint8_t *data, 
  size_t length){

  return i2c_write_read(MPU6050_ADDR, &reg, 1, data, length) ==
  i2cTransferDone;
}

static int16_t to_int16(const uint8_t *d){
    return (int16_t)((d[0] << 8) | d[1]);
}

bool mpu6050_init(void){
  uint8_t id = 0;

  //1. check connection
  if (!read_regs(REG_WHO_AM_I, &id, 1) || id != 0x68) return false;

  //2. reset (DEVICE_RESET bit 7), wait for it to restart
  if (!write_reg(REG_PWR_MGMT_1, 0x80)) return false;
  sl_sleeptimer_delay_millisecond(100);

  //3. wake up (SLEEP = 0), clock = PLL with X gyro (CLKSEL = 1)
  if (!write_reg(REG_PWR_MGMT_1, 0x01)) return false;

  //4. DLPF_CFG = 0:  gyro output rate 8 kHz
  if (!write_reg(REG_CONFIG, 0x00)) return false;

  //5. sample rate = 8 kHz / (1 + 3) = 2 kHz
  if (!write_reg(REG_SMPLRT_DIV, 3)) return false;

  //6. gyro +-500 deg/s (FS_SEL = 1)
  if (!write_reg(REG_GYRO_CONFIG, 0x08)) return false;

  //7. accel +-4g (AFS_SEL = 1)
  if (!write_reg(REG_ACCEL_CONFIG, 0x08)) return false;

  return true;
}

bool mpu6050_read_accel(
  int32_t *x_mg, 
  int32_t *y_mg, 
  int32_t *z_mg){

  uint8_t d[6];
  if(!read_regs(REG_ACCEL_XOUT_H, d, 6)) return false;

  *x_mg = (int32_t)to_int16(&d[0]) * 1000 / ACCEL_LSB_PER_G;
  *y_mg = (int32_t)to_int16(&d[2]) * 1000 / ACCEL_LSB_PER_G;
  *z_mg = (int32_t)to_int16(&d[4]) * 1000 / ACCEL_LSB_PER_G;
  return true;
}

bool mpu6050_read_gyro(
  int32_t *x_mdps,
  int32_t *y_mdps){

  uint8_t d[4];   // X_H X_L Y_H Y_L
  if (!read_regs(REG_GYRO_XOUT_H, d, 4)) return false;

  *x_mdps = (int32_t)to_int16(&d[0]) * 10000 / GYRO_LSB_PER_DPS;
  *y_mdps = (int32_t)to_int16(&d[2]) * 10000 / GYRO_LSB_PER_DPS;
  return true;

}

bool mpu6050_read_temp(int32_t *t_centi){
    uint8_t d[2];
    if (!read_regs(REG_TEMP_OUT_H, d, 2)) return false;

    *t_centi = (int32_t)to_int16(d) * 100 / 340 + 3653;
    return true;
}


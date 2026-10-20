#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include <stdint.h>

#define MPU6050_ADDR 0x68

bool mpu6050_init(void);
bool mpu6050_read_accel(int32_t *x_mg, int32_t *y_mg, int32_t *z_mg);
bool mpu6050_read_gyro(int32_t *x_mdps, int32_t *y_mdps);
bool mpu6050_read_temp(int32_t *t_centi);

#endif

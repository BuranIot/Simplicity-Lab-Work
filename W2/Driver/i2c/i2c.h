#ifndef I2C_H
#define I2C_H

#include <stddef.h>
#include <stdint.h>

#include "em_i2c.h"

// I2C0 on SCL = PB00, SDA = PB01, 100 kHz master
void i2c_init(void);

// write data with return type
I2C_TransferReturn_TypeDef i2c_write(uint8_t address,
                                    const uint8_t *data,
                                    size_t length);

I2C_TransferReturn_TypeDef i2c_read(uint8_t address,
                                    uint8_t *data,
                                    size_t length);

I2C_TransferReturn_TypeDef i2c_write_read(uint8_t address,
                                          const uint8_t *write_data,
                                          size_t write_length,
                                          uint8_t *read_data,
                                          size_t read_length);

I2C_TransferReturn_TypeDef i2c_write_register(uint8_t address,
                                               uint8_t reg,
                                               uint8_t value);

I2C_TransferReturn_TypeDef i2c_read_registers(uint8_t address,
                                               uint8_t start_reg,
                                               uint8_t *data,
                                               size_t length);

#endif

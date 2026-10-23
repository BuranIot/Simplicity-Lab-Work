#include "i2c.h"
#include "em_cmu.h"
#include "em_gpio.h"

#define I2C_TIMEOUT 10000u

#define I2C_SCL_PORT gpioPortB
#define I2C_SCL_PIN 0
#define I2C_SDA_PORT gpioPortB
#define I2C_SDA_PIN 1

void i2c_init(void){

    //Enable clock for GPIO and I2C_H
    CMU_ClockEnable(cmuClock_GPIO, true);
    CMU_ClockEnable(cmuClock_I2C0, true);

    //SCL and SDA pullup
    GPIO_PinModeSet(I2C_SCL_PORT, I2C_SCL_PIN, gpioModeWiredAndPullUp, 1);
    GPIO_PinModeSet(I2C_SDA_PORT, I2C_SDA_PIN, gpioModeWiredAndPullUp, 1);

    //GPIO routing and enable pin
    GPIO -> I2CROUTE[0].SCLROUTE = (I2C_SCL_PORT << _GPIO_I2C_SCLROUTE_PORT_SHIFT) |
    (I2C_SCL_PIN << _GPIO_I2C_SCLROUTE_PIN_SHIFT);
    GPIO -> I2CROUTE[0].SDAROUTE = (I2C_SDA_PORT << _GPIO_I2C_SDAROUTE_PORT_SHIFT) |
    (I2C_SDA_PIN << _GPIO_I2C_SDAROUTE_PIN_SHIFT);    
    GPIO -> I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SCLPEN |
    GPIO_I2C_ROUTEEN_SDAPEN;

    //master mode
    I2C_Init_TypeDef init = I2C_INIT_DEFAULT;
    I2C_Init(I2C0, &init);
}

static uint32_t wait_flag(uint32_t mask){

    uint32_t count = 0;
    uint32_t flags = 0;
    //if ACK not confirm it will return as timeout
    while(count < I2C_TIMEOUT){
        flags = I2C0 -> IF & mask;
            if (flags != 0){    
                (I2C0 -> IF_CLR = flags);
                return flags;
            }
        count++;
    }
    return 0;

}

static void i2c_stop(void){
    I2C0 -> CMD = I2C_CMD_STOP;
    wait_flag(I2C_IF_MSTOP);
}

static I2C_TransferReturn_TypeDef i2c_fail(uint32_t f){
    i2c_stop();
    return(f & I2C_IF_NACK) ? i2cTransferNack : i2cTransferSwFault;
}

I2C_TransferReturn_TypeDef i2c_write_read(
    uint8_t address, 
    const uint8_t *write_data,
    size_t write_length,
    uint8_t *read_data,
    size_t read_length){

    uint32_t flags;
    
    // clear old flag, TX buffer and pending
    I2C0 -> IF_CLR = _I2C_IF_MASK;
    I2C0 -> CMD = I2C_CMD_CLEARTX | I2C_CMD_CLEARPC;

    //Start + address with W bit
    I2C0 -> TXDATA = (uint32_t)(address << 1);
    I2C0 -> CMD = I2C_CMD_START;
    flags = wait_flag(I2C_IF_ACK | I2C_IF_NACK);
    if (flags != I2C_IF_ACK) return i2c_fail(flags);

    //send each data byte
    for(size_t i = 0; i < write_length; i++){
        I2C0 -> TXDATA = write_data[i];
        flags = wait_flag(I2C_IF_ACK | I2C_IF_NACK);
        if (flags != I2C_IF_ACK) return i2c_fail(flags);
    }

    //receive Byte
    I2C0 -> CMD = I2C_CMD_START;
    I2C0 -> TXDATA = (uint32_t)((address << 1) | 1);
    flags = wait_flag(I2C_IF_ACK | I2C_IF_NACK);
    if (flags != I2C_IF_ACK) return i2c_fail(flags);
    for(size_t i = 0; i < read_length; i++){
        if (wait_flag(I2C_IF_RXDATAV) == 0) return i2c_fail(0);
        read_data[i] = (uint8_t)I2C0 -> RXDATA;
        I2C0 -> IF_CLR = I2C_IF_RXDATAV;
        if(i < read_length - 1){
            I2C0 -> CMD = I2C_CMD_ACK;
        } else{
            I2C0 -> CMD = I2C_CMD_NACK;
        }
    }

    //Stop and wait until it sent
    I2C0 -> CMD = I2C_CMD_STOP;
    if(wait_flag(I2C_IF_MSTOP) == 0) {
        return i2cTransferSwFault;
    }
    
    return i2cTransferDone;
}

I2C_TransferReturn_TypeDef i2c_write_register(
    uint8_t address,
    uint8_t reg,
    uint8_t value){

    uint32_t flags;
    uint8_t buf[2] = {reg, value};

    // clear old flag, TX buffer and pending
    I2C0 -> IF_CLR = _I2C_IF_MASK;
    I2C0 -> CMD = I2C_CMD_CLEARTX | I2C_CMD_CLEARPC;

    //Start + address with W bit
    I2C0 -> TXDATA = (uint32_t)(address << 1);
    I2C0 -> CMD = I2C_CMD_START;
    flags = wait_flag(I2C_IF_ACK | I2C_IF_NACK);
    if (flags != I2C_IF_ACK) return i2c_fail(flags);

    //send register address, then value
    for(size_t i = 0; i < 2; i++){
        I2C0 -> TXDATA = buf[i];
        flags = wait_flag(I2C_IF_ACK | I2C_IF_NACK);
        if (flags != I2C_IF_ACK) return i2c_fail(flags);
    }

    //Stop and wait until it sent
    I2C0 -> CMD = I2C_CMD_STOP;
    if(wait_flag(I2C_IF_MSTOP) == 0) {
        return i2cTransferSwFault;
    }

    return i2cTransferDone;
}

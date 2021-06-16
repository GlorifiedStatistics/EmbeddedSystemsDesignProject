#ifndef I2C_H
#define I2C_H

#define BAUD_RATE 200

#define WRITE_DIR 0
#define READ_DIR 1

void i2c_init();
uint8_t i2c_transaction(uint8_t address, uint8_t dir, uint8_t* data, uint8_t len);

#endif

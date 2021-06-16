#ifndef BMA_250_H
#define BMA_250_H

#define BMA_ADDRESS 0x19
#define BUF_SIZE 10

#define X_LSB 0x02
#define Y_LSB 0x04
#define Z_LSB 0x06

void bma250_init();
void bma250_read_xyz(int16_t* x, int16_t* y, int16_t* z);

#endif

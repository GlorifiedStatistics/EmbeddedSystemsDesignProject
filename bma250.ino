#include <sam.h>
#include "bma250.h"
#include "i2c.h"

static uint8_t buf[BUF_SIZE];

void bma250_init() {
  for (unsigned int i = 0; i < BUF_SIZE; i++){
    buf[i] = 0;
  }
}

int16_t getVal(uint8_t addr){
  int16_t ret = 0;
  
  // Get LSB and MSB
  i2c_transaction(BMA_ADDRESS, WRITE_DIR, &addr, 1);
  i2c_transaction(BMA_ADDRESS, READ_DIR, buf, 2);
  ret = (buf[0] >> 6) + (buf[1] << 2);

  // Fix the two's complement
  if (ret & 0x0200) {
    ret = -(((~ret) & 0x01FF) + 1);
  }

  return ret;
}

void bma250_read_xyz(int16_t* x, int16_t* y, int16_t* z){
  *x = getVal(X_LSB);
  *y = getVal(Y_LSB);
  *z = getVal(Z_LSB);

  /*
  // TESTING
  addr = 0x0;
  i2c_transaction(BMA_ADDRESS, WRITE_DIR, &addr, 1);
  i2c_transaction(BMA_ADDRESS, READ_DIR, buf, 3);
  *x = buf[0];
  *y = buf[1];
  *z = buf[2];
  */
}

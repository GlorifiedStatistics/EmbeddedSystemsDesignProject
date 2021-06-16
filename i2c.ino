#include <sam.h>
#include "i2c.h"

// Do all the initialization
void i2c_init(){
  // Setup the pins
  PORT->Group[0].PINCFG[PIN_PA22].bit.PMUXEN = 1;
  PORT->Group[0].PINCFG[PIN_PA23].bit.PMUXEN = 1;
  PORT->Group[0].PMUX[11].bit.PMUXE = 0x2;
  PORT->Group[0].PMUX[11].bit.PMUXO = 0x2;

  // Setup the clock
  GCLK->CLKCTRL.bit.ID = GCLK_CLKCTRL_ID_SERCOM3_CORE_Val;    // select SERCOM3 peripheral channel
  GCLK->CLKCTRL.bit.GEN = 0;        // select source GCLK_GEN[0]
  GCLK->CLKCTRL.bit.CLKEN = 1;            // enable clock
  PM->APBCSEL.bit.APBCDIV = 0;            // no prescaler
  PM->APBCMASK.bit.SERCOM3_ = 1;                // enable SERCOM3 interface
  
  // Setup SERCOM3
  // Do software reset at beginning just in case
  SERCOM3->I2CM.CTRLA.bit.SWRST = 1;
  while(SERCOM3->I2CM.CTRLA.bit.SWRST || SERCOM3->I2CM.SYNCBUSY.bit.SWRST);
  
  SERCOM3->I2CM.CTRLA.bit.ENABLE = 0; // Disable first
  SERCOM3->I2CM.CTRLA.bit.MODE = 0x5; // Set as master
  SERCOM3->I2CM.CTRLA.bit.SCLSM = 0; // Turn off clock stretch
  SERCOM3->I2CM.CTRLA.bit.SPEED = 0; // Set to lowest speed (for 100khz baud)
  SERCOM3->I2CM.CTRLA.bit.INACTOUT = 0; // Turn off inactive timeout
  
  SERCOM3->I2CM.CTRLB.bit.SMEN = 1; // Enable smart mode for acknowledge bits
  SERCOM3->I2CM.BAUD.bit.BAUDLOW = 0; // Set baudlow to 0 for BAUD to control both high and low
  SERCOM3->I2CM.BAUD.bit.BAUD = BAUD_RATE; // Set baud rate
  SERCOM3->I2CM.CTRLA.bit.ENABLE = 1; // Enable
}

// Returns 0 if it worked, 1 if error
uint8_t i2c_transaction(uint8_t address, uint8_t dir, uint8_t* data, uint8_t len){
  uint8_t ret;  // The return value
  
  // Set busstate to 1 if it is currently at 0
  if (SERCOM3->I2CM.STATUS.bit.BUSSTATE == 0) SERCOM3->I2CM.STATUS.bit.BUSSTATE = 1;

  // Enter the address (shifted left 1 bit) or-ed with dir (0 for write, 1 for read)
  SERCOM3->I2CM.ADDR.bit.ADDR = (address << 1) | dir;

  // Wait for the SERCOM to finish
  while(SERCOM3->I2CM.SYNCBUSY.bit.SYSOP);
  if (dir == WRITE_DIR) while(!SERCOM3->I2CM.INTFLAG.bit.MB);
  else while(!SERCOM3->I2CM.INTFLAG.bit.SB);

  // Loop through the length, either writing bytes from data to the given address, or reading them into data
  //   then, wait for each byte to finish sending/reading
  for (uint8_t i = 0; i < len; i++){
    if (dir == WRITE_DIR) {
      SERCOM3->I2CM.DATA.bit.DATA = data[i];
      while(SERCOM3->I2CM.SYNCBUSY.bit.SYSOP);
      while(!SERCOM3->I2CM.INTFLAG.bit.MB);
    }
    else {
      data[i] = SERCOM3->I2CM.DATA.bit.DATA;
      while(SERCOM3->I2CM.SYNCBUSY.bit.SYSOP);
      while(!SERCOM3->I2CM.INTFLAG.bit.SB);
    }
  }

  // Return 1 if error, 0 if no error
  ret = SERCOM3->I2CM.INTFLAG.bit.ERROR;
 
  // Send NACK if reading
  if (dir == READ_DIR){
    SERCOM3->I2CM.CTRLB.bit.ACKACT = 1;  // Set to 1 to send NACK instead of ACK
    SERCOM3->I2CM.CTRLB.bit.CMD = 0x2;  // Send NACK
    while(SERCOM3->I2CM.SYNCBUSY.bit.SYSOP);  // Wait for NACK to send
    while(!SERCOM3->I2CM.INTFLAG.bit.SB);
    SERCOM3->I2CM.CTRLB.bit.ACKACT = 0;  // Reset to sending ACK instead of NACK
  }

  // Send STOP message
  SERCOM3->I2CM.CTRLB.bit.CMD = 0x3;

  // Wait to return to IDLE state
  while(SERCOM3->I2CM.STATUS.bit.BUSSTATE > 1);

  return ret;
}

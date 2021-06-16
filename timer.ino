#include "timer.h"


// Code blatantly copied from: https://microchipdeveloper.com/32arm:samd21-nvic-configuration
void timer3_init() {

  // Configure asynchronous clock source
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_TCC2_TC3_Val;    // select TC3 peripheral channel
  GCLK->CLKCTRL.reg |= GCLK_CLKCTRL_GEN_GCLK0;        // select source GCLK_GEN[0]
  GCLK->CLKCTRL.bit.CLKEN = 1;            // enable TC3 generic clock
  PM->APBCSEL.bit.APBCDIV = 0;            // no prescaler
  PM->APBCMASK.bit.TC3_ = 1;                // enable TC3 interface
  
  // Configure Count Mode (32-bit)
  TC3->COUNT32.CTRLA.bit.MODE = 0x2;
  
  // Configure Prescaler for divide by 2 (3 MHz clock to COUNT)
  TC3->COUNT32.CTRLA.bit.PRESCALER = 0x6;
  
  TC3->COUNT32.CTRLA.bit.WAVEGEN = 0x1;            // "Match Frequency" operation

  // Set an arbitrary desired speed initially
  TC3->COUNT32.CC[0].reg = 1 << 15;
  
  // Enable TC3 compare mode interrupt generation
  TC3->COUNT32.INTENSET.bit.MC0 = 0x1;    // Enable match interrupts on compare channel 0 
  
  // Enable TC3
  TC3->COUNT32.CTRLA.bit.ENABLE = 1;
  
  // Wait until TC3 is enabled
  while(TC3->COUNT32.STATUS.bit.SYNCBUSY == 1);

  // Setup interrupts
  NVIC_SetPriority(TC3_IRQn, 3);
  NVIC_EnableIRQ(TC3_IRQn);
}

void timer3_reset(){
  TC3->COUNT32.COUNT.reg = 0;
}

// Reset timer after changing speed
void timer3_set(uint16_t period_ms){
  TC3->COUNT32.CC[0].reg = CLOCK_SPEED / 256 * period_ms / 1000;
  timer3_reset();
}

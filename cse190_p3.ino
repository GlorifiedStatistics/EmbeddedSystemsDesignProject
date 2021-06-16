#include <sam.h>

#include "timer.h"
#include "i2c.h"
#include "bma250.h"
#include <SPI.h>
#include "STBLE.h"
#include "ble.h"

#define PIPE_UART_OVER_BTLE_UART_TX_TX 0

#define ACC_DIST 10
#define CHK_DIST 3
#define ACC_DELTA 1200
#define BLE_WAIT_SECS 10

#define BUF_SIZE 256
#define MAX_MESSAGE_SIZE 11

// Type to store clock ticks
typedef uint64_t tick_t;

// Redefine the newlib libc _write() function so you can use printf in your code
extern "C" int _write(int fd, const void *buf, size_t count) {
  // STDOUT
  if (fd == 1)
    SerialUSB.write((char*)buf, count);
  return 0;
}


/* === DO NOT REMOVE: Initialize C library === */
extern "C" void __libc_init_array(void);

static int16_t ax[ACC_DIST], ay[ACC_DIST], az[ACC_DIST];
static uint16_t accIdx = 0xFFFF;  // Start here to overflow to 0 on first checkDrop call
static bool fullRot = false, dropped = false, fullTime = false, slept = true;

static tick_t ticks = 0;


void TC3_Handler(void) {
  // Acknowledge the interrupt (clear MC0 interrupt flag to re-arm)
  TC3->COUNT16.INTFLAG.reg |= 0b00010000;

  // Increment the ticks
  ticks ++;
}

bool checkDrop(int16_t x, int16_t y, int16_t z){
  // Copy in the new data
  accIdx = (accIdx + 1) % ACC_DIST;
  ax[accIdx] = x;
  ay[accIdx] = y;
  az[accIdx] = z;

  
  // If we haven't filled up the array yet, don't do anything but copy data
  if (!fullRot) {
    // Check if we are done filling up
    if (accIdx == ACC_DIST - 1) fullRot = true;
    else return false;
  }

  // Check if there has been, in general, a large change in acceleration in this ACC_DIST
  int16_t sx = 0, sy = 0, sz = 0;
  unsigned int i;
  for (i = 0; i < ACC_DIST - CHK_DIST; i++){
    sx += ax[(accIdx + i) % ACC_DIST];
    sy += ay[(accIdx + i) % ACC_DIST];
    sz += az[(accIdx + i) % ACC_DIST];
  }
  sx /= (ACC_DIST - CHK_DIST);
  sy /= (ACC_DIST - CHK_DIST);
  sz /= (ACC_DIST - CHK_DIST);

  int16_t tx = 0, ty = 0, tz = 0;
  for (; i < ACC_DIST; i++){
    tx += ax[(accIdx + i) % ACC_DIST];
    ty += ay[(accIdx + i) % ACC_DIST];
    tz += az[(accIdx + i) % ACC_DIST];
  }
  tx /= CHK_DIST;
  ty /= CHK_DIST;
  tz /= CHK_DIST;

  return ( abs(sx - tx) + abs(sy - ty) + abs(sz - tz ) ) > ACC_DELTA;
}

void sendMessage(uint8_t* message){
  // Calculate message size
  uint8_t bufSize = 1;
  for (unsigned int i = 0; i < BUF_SIZE; i++){
    if (message[i] == 0) break;
    bufSize++;
  }

  uint8_t start = 0;
  while (bufSize > 0){
    uint8_t sendSize = min(bufSize, MAX_MESSAGE_SIZE);
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, &(message[start]), sendSize)){
      printf("Failure sending bytes!\n");
    }
    start += sendSize;
    bufSize -= sendSize;
  }
}

// Put the BLE to sleep using a barbaric whole-device reset since I couldn't figure out how to simply disconnect from a host
//  Then, once the device has been reset, go into standby mode
void sleepBLE(){
  printf("Putting BLE to sleep\n");
  BlueNRG_RST();
  if (aci_hal_device_standby()){
    printf("ERROR SLEEPING\n");
  }
  slept = true;
}

int main(void) {  
  /* ==== DO NOT REMOVE: USB configuration ==== */
  init();
  __libc_init_array();
  USBDevice.init();
  USBDevice.attach();
  /* =========================================== */
  
  timer3_init();
  i2c_init();
  bma250_init();
  ble_init();
  sleepBLE();

  // Interrupt 4 times a second since that's the quickest I can easily do, and I can
  //  simply bit-shift right by 2 to get number of seconds
  timer3_set(1000/4);
  
  /* === Main Loop === */

  int16_t x,y,z;
  uint8_t message[BUF_SIZE];
  uint8_t ledTime;

  tick_t lastTime = 0;
  
  while (1) {
    
    // If we haven't been dropped yet, check for drop
    if (!dropped){
      bma250_read_xyz(&x, &y, &z);
      if(checkDrop(x, y, z)){

        // Set dropped to true, and reset ticks
        dropped = true;
        ticks = 0;
        printf("DROPPED!\n");
      }
    }

    // otherwise, light up some LED's
    else{

      // Check if this is the first time waking up
      if (slept) {
        // Redo the ble_init() step since we probably reset the device on last sleep
        ble_init();
        setBLEConnectable();  // We also have to set the set_connectable value to 1 in ble.cpp
        slept = false;
      }
      
      // Process bluetooth loop
      ble_loop();
      
      // If we have not reached the full 255 minutes, then check the actual time
      if (!fullTime) {
        // Shift ticks right by 2 to get number of seconds, then divide by 60 to get minutes
        tick_t numMins = (ticks >> 2) / 60;

        // If we have finally reached 255 minutes, set fullTime to true to not have to compute this again
        if (numMins > 255) {
          fullTime = true;
          ledTime = 255;
        }else{
          ledTime = (uint8_t) numMins;
        }
      }

      // Otherwise, just set to 255 to avoid any eventual overflow (even if it would take ~150 billion years...)
      else{
        ledTime = 255;
      }

  
      if (ticks - lastTime > BLE_WAIT_SECS << 2) {
        if (ticks < 60 << 2){
          if (ticks >> 2 > 1){
            sprintf((char*)message, "Tag 'A14566071' dropped %d seconds ago", ticks >> 2);
          }else{
            sprintf((char*)message, "Tag 'A14566071' dropped %d second ago", ticks >> 2);
          }
        }else{
          if (ledTime > 1){
            sprintf((char*)message, "Tag 'A14566071' dropped %d minutes ago", ledTime);
          }else{
            sprintf((char*)message, "Tag 'A14566071' dropped %d minute ago", ledTime);
          }
        }
        sendMessage(message);
        lastTime = ticks;
      }

      // If we get the reset signal, reset things
      if (isStopTransmitting()){
        printf("Recieved stop signal\n");
        delay(10);
        dropped = 0;
        fullRot = false;
        fullTime = false;
        accIdx = 0xFFFF;
        sleepBLE();
      }
    }
    
  }
  
  return 0;
}

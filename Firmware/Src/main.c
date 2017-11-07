/**
 */
#include "platform_hw.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_i2c.h"
#include "iprintf.h"

#include "led.h"
#include "accelerometer.h"
#include "board_id.h"
#include "version.h"

#include <string.h>
#include <stdlib.h>

#define     ADDR_TOP       (0xA0)

//FIXME move elsewhere
extern I2C_HandleTypeDef hi2c1;

int main(void)
{
   // Reset of all peripherals, Initializes the Flash interface and the Systick
   HAL_Init();

   platformHW_Init();

   iprintf("\r\nStarting... (v%d | #0x%x / 0x%x | Built "__DATE__":"__TIME__")\r\n", FW_VERSION, bid_GetID(), bid_GetIDCrc());

   //led_Init();

   //accelerometer_Init();
   //accelerometer_TestStream();

   HAL_StatusTypeDef stat;

   iprintf("Starting EEPROM Test...\n");

   uint8_t baddr = ADDR_TOP | (0x0 << 1);
   uint8_t dataOut[] = "This is a test string!";
   uint8_t dataIn[sizeof(dataOut)] = {};
   stat = HAL_I2C_Mem_Read(&hi2c1, baddr, 0x0, 1, dataIn, sizeof(dataIn), 1000);
   iprintf("Stat = 0x%x\n", stat);
   iprintf("Read [%s]\n", dataIn);

   // mask 3 block select bits in <<1
   stat = HAL_I2C_Mem_Write(&hi2c1, baddr, 0x0, 1, dataOut, sizeof(dataOut), 1000);
   iprintf("Stat = 0x%x\n", stat);
   iprintf("Wrote [%s]\n", dataOut);

   //TODO spin trying to initiate a write until we get an ACK (indicating the last
   //write completed)

   //TODO clock enable broken or something?
   //HAL_Delay(100);

   stat = HAL_I2C_Mem_Read(&hi2c1, baddr, 0x0, 1, dataIn, sizeof(dataIn), 1000);
   iprintf("Stat = 0x%x\n", stat);
   iprintf("Read [%s]\n", dataIn);

   return 0;
}


void main_ButtonCB(void) {
   //HAL_GetTick();
   iprintf("Button!\n");
}


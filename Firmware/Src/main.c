/**
 */
#include "platform_hw.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_i2c.h"
#include "iprintf.h"

#include "led.h"
#include "accelerometer.h"
#include "eeprom.h"
#include "board_id.h"
#include "version.h"

#include <string.h>
#include <stdlib.h>

int main(void)
{
   // Reset of all peripherals, Initializes the Flash interface and the Systick
   HAL_Init();

   platformHW_Init();

   iprintf("\r\nStarting... (v%d | #0x%x / 0x%x | Built "__DATE__":"__TIME__")\r\n", FW_VERSION, bid_GetID(), bid_GetIDCrc());

   //led_Init();

   //accelerometer_Init();
   //accelerometer_TestStream();

   //FIXME rm
   iprintf("Starting EEPROM Test...\n");

   uint8_t dataOut[] = "This is a test string!";
   uint8_t dataIn[sizeof(dataOut)] = {};

   eeprom_Write(0x0, dataOut, sizeof(dataOut));
   eeprom_Read(0x0, dataIn, sizeof(dataIn));

   iprintf("Wrote [%s]\n", dataOut);
   iprintf("Read [%s]\n", dataIn);

   return 0;
}


void main_ButtonCB(void) {
   //HAL_GetTick();
   iprintf("Button!\n");
}


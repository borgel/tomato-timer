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

union Interrupts {
   uint32_t mask;
   struct {
      uint8_t     accelerometer  : 1;
      uint8_t     charger        : 1;
   };
};

static union Interrupts ints;

int main(void)
{
   // Reset of all peripherals, Initializes the Flash interface and the Systick
   HAL_Init();

   //HAL_GetTick();

   platformHW_Init();

   iprintf("\r\nStarting... (v%d | #0x%x / 0x%x | Built "__DATE__":"__TIME__")\r\n", FW_VERSION, bid_GetID(), bid_GetIDCrc());

   led_Init();

   accelerometer_Init();
   //accelerometer_TestOrientation();
   //accelerometer_TestStream();

   /*
   for(int i = 0; i < 36; i++) {
      led_SetChannel(i, 122);
      HAL_Delay(200);
   }
   */

   while(1) {
      // Process all pending interrupts
      while(ints.mask) {
         if(ints.accelerometer) {
            ints.accelerometer = 0;

            accelerometer_DecodeInterrupt();

            led_SetChannel(7, 122);
         }
      }

      //TODO goto WFI?
      HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
   }

   return 0;
}

void main_SetAcceleInt(void) {
   ints.accelerometer = 1;
}


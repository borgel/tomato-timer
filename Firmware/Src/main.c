/**
 */
#include "platform_hw.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_i2c.h"
#include "iprintf.h"

#include "board_id.h"
#include "version.h"

#include <string.h>
#include <stdlib.h>

//FIXME move elsewhere
extern I2C_HandleTypeDef hi2c1;
// can only write (0b0)
#define LED_CONT_ADDR      (0x78)

int main(void)
{
   // Reset of all peripherals, Initializes the Flash interface and the Systick
   HAL_Init();

   platformHW_Init();

   iprintf("\r\nStarting... (v%d | #0x%x / 0x%x | Built "__DATE__":"__TIME__")\r\n", FW_VERSION, bid_GetID(), bid_GetIDCrc());

   //TODO send i2c turn on LEDs
   //HAL_I2C_Master_Transmit
   //HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

   HAL_StatusTypeDef stat;
   uint8_t data[63 + 10] = {};

   // disable SW shutdown
   data[0] = 0x0;
   data[1] = 0x1;
   stat = HAL_I2C_Master_Transmit(&hi2c1, LED_CONT_ADDR, data, 2, 1000);
   iprintf("Stat = 0x%x\n", stat);

   HAL_Delay(100);

   // set all channels to mid brightness
   data[0] = 0x1;
   for(int i = 0; i < 36; i++) {
      data[1 + i] = 255/1;
   }
   stat = HAL_I2C_Master_Transmit(&hi2c1, LED_CONT_ADDR, data, 37, 1000);
   iprintf("Stat = 0x%x\n", stat);

   HAL_Delay(100);

   // set enable bit on all channels
   data[0] = 0x26;
   for(int i = 0; i < 36; i++) {
      //100% bright
      //data[1 + i] = 0x1;

      //1/4 bright
      data[1 + i] = 0x1 | (0x3 << 1);
   }
   stat = HAL_I2C_Master_Transmit(&hi2c1, LED_CONT_ADDR, data, 37, 1000);
   iprintf("Stat = 0x%x\n", stat);

   HAL_Delay(100);

   // write update bit to commit all values
   data[0] = 0x25;
   data[1] = 0x0;
   stat = HAL_I2C_Master_Transmit(&hi2c1, LED_CONT_ADDR, data, 2, 1000);
   iprintf("Stat = 0x%x\n", stat);

   HAL_Delay(100);

   // enable all channels
   data[0] = 0x4A;
   data[1] = 0x0;
   stat = HAL_I2C_Master_Transmit(&hi2c1, LED_CONT_ADDR, data, 2, 1000);


   while(1)
   {
   }

   return 0;
}


void main_ButtonCB(void) {
   //HAL_GetTick();
   iprintf("Button!\n");
}


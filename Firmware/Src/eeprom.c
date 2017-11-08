#include "eeprom.h"

#include "platform_hw.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_i2c.h"
#include "iprintf.h"

#include <stdbool.h>
#include <stdint.h>

#define     EEPROM_ADDR          (0xA0)
#define     PAGE_SIZE_BYTES      (8)

//FIXME find a better way to share this
extern I2C_HandleTypeDef hi2c1;

// Shared buffer used for reads and writes. The page size is so small it doesn't
// really matter if we keep this around
static uint8_t mOperationBuffer[PAGE_SIZE_BYTES + 1];

bool eeprom_Write(uint8_t addr, uint8_t * buffer, unsigned length) {
   HAL_StatusTypeDef stat;

   (void)mOperationBuffer;

   //TODO chunk into 8 byte pages

   stat = HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, addr, 1, buffer, length, 1000);
   iprintf("Stat = 0x%x\n", stat);

   stat = HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, addr, 1, buffer, length, 1000);
   iprintf("Stat = 0x%x\n", stat);

   //TODO spin trying to initiate a write until we get an ACK (indicating the last
   //write completed)

   //TODO clock enable broken or something?
   //HAL_Delay(100);

   stat = HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, addr, 1, buffer, length, 1000);
   iprintf("Stat = 0x%x\n", stat);

   return true;
}

bool eeprom_Read(uint8_t addr, uint8_t * buffer, unsigned length) {
   //TODO chunk into 8 byte pages

   return false;
}

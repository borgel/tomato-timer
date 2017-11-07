#include "accelerometer.h"

#include "platform_hw.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_i2c.h"
#include "iprintf.h"

#include <stdint.h>

#define        ACCELE_ADDR       (0x39)

#define        WHO_AM_I_VAL      (0x2A)

#define        REG_STATUS        (0x00)
#define        REG_OUT_X_MSB     (0x01)   //0x01 thru 0x06 are X/Y/Z
#define        REG_WHO_AM_I      (0x0D)
#define        REG_PL_STATUS     (0x10)
#define        REG_PL_CFG        (0x11)
#define        REG_CTRL1         (0x2A)

//FIXME move elsewhere
extern I2C_HandleTypeDef hi2c1;

bool accelerometer_Init(void) {
   HAL_StatusTypeDef stat;

   uint8_t idReg = 0;
   stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_WHO_AM_I, 1, &idReg, 1, 1000);
   iprintf("Stat = 0x%x\n", stat);
   iprintf("ID Reg = 0x%x (want 0x2A)\n", idReg);

   if(idReg != WHO_AM_I_VAL) {
      iprintf("Acceleromete: Who Am I value mismatch! Got 0x%x expected 0x%x\n", idReg, WHO_AM_I_VAL);
      return false;
   }
   return true;
}

void accelerometer_TestStream(void) {
   HAL_StatusTypeDef stat;

   //enter active mode?
   uint8_t regCtrl1 = (0x2 << 5) | 0x1;
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_CTRL1, 1, &regCtrl1, 1, 1000);
   iprintf("Stat = 0x%x\n", stat);

   HAL_Delay(100);

   // 3 x 2 bytes (12 bits)
   uint8_t data[6];
   while(1) {
      HAL_Delay(50);

      stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_OUT_X_MSB, 6, data, 6, 1000);
      if(stat != 0) {
         iprintf("Stat = 0x%x\n", stat);

         /*
         uint8_t regStatus = 0;
         stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_STATUS, 1, &regStatus, 1, 1000);
         iprintf("Stat = 0x%x\n", stat);
         iprintf("Status Reg = 0x%x\n", regStatus);
         */
         continue;
      }

      for(int i = 0; i < sizeof(data); i++) {
         iprintf("0x%x ", data[i]);
      }
      iprintf("\n");

      //TODO unpack values

   }
}

void accelerometer_TestOrientation(void) {
   HAL_StatusTypeDef stat;

   //enable port/land mode 0x11
   uint8_t regPlCFG = 0x1 << 6;
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_PL_CFG, 1, &regPlCFG, 1, 1000);
   iprintf("Enable PL Stat = 0x%x\n", stat);

   HAL_Delay(100);

   while (1) {
      HAL_Delay(500);

      // print port/land
      uint8_t regOrient;
      stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_PL_STATUS, 1, &regOrient, 1, 1000);
      iprintf("PL Stat = 0x%x\n", stat);
      iprintf("Orient = 0x%x\n", regOrient);
   }

}

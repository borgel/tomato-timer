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
#define        REG_PL_COUNT      (0x12)
#define        REG_PL_BF_ZCOMP   (0x13)
#define        REG_PL_THS_REG    (0x14)
#define        REG_CTRL1         (0x2A)
#define        REG_CTRL2         (0x2B)
#define        REG_CTRL3         (0x2C)
#define        REG_CTRL4         (0x2D)
#define        REG_CTRL5         (0x2E)

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

static bool _SetEnterStandby(bool enter) {
   HAL_StatusTypeDef stat;
   uint8_t reg = 0;

   stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_CTRL1, 1, &reg, 1, 1000);
   iprintf("Standby 1 Stat = 0x%x\n", stat);

   // set or clear the standby bit (0)
   if(enter) {
      reg &= 0xFE;
   }
   else {
      reg |= 0x1;
   }

   // write it back
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_CTRL1, 1, &reg, 1, 1000);
   iprintf("Standby 2 Stat = 0x%x\n", stat);

   // True on success
   return (stat == 0);
}

//FIXME move
enum SampleRate {
   SAMPLE_RATE_50HZ = 0x20,
};

static bool _SetSampleRate(enum SampleRate rate) {
   HAL_StatusTypeDef stat;
   uint8_t reg = 0;

   stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_CTRL1, 1, &reg, 1, 1000);
   iprintf("Standby 1 Stat = 0x%x\n", stat);

   // clear sample rate bits
   reg &= 0xC7;
   reg |= rate;

   // write it back
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_CTRL1, 1, &reg, 1, 1000);
   iprintf("Standby 2 Stat = 0x%x\n", stat);

   // True on success
   return (stat == 0);
}

//FIXME move
enum Interrupts {
   INT_EN_LNDPRT   = (0x1 << 4),
};

// this is a little weird. in each of the 8 positions (matching above), a 0 means int2, 1 is int1
enum InterruptLine {
   INT_EN_LNDPRT_INT1   = (0x1 << 4),
   INT_EN_LNDPRT_INT2   = (0x0 << 4),
};

// expects a mask of enabled interrupts
static bool _SetEnabledInterrupts(enum Interrupts enabledInterrupts, enum InterruptLine enabledLines) {
   HAL_StatusTypeDef stat;
   uint8_t reg = 0;

   // don't bother reading CTRL4 or CRTL5, we are about to wipe them out

   reg = enabledInterrupts;

   // write it back
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_CTRL4, 1, &reg, 1, 1000);
   iprintf("Standby 2 Stat = 0x%x\n", stat);

   reg = enabledLines;

   // write it back
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_CTRL5, 1, &reg, 1, 1000);
   iprintf("Standby 2 Stat = 0x%x\n", stat);

   // True on success
   return (stat == 0);
}

void accelerometer_TestOrientation(void) {
   HAL_StatusTypeDef stat;

   _SetEnterStandby(true);

   //set sample rate to 50Hz
   _SetSampleRate(SAMPLE_RATE_50HZ);

   //enable port/land mode 0x11
   uint8_t regPlCFG = 0x1 << 6;
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_PL_CFG, 1, &regPlCFG, 1, 1000);
   iprintf("Enable PL Stat = 0x%x\n", stat);

   // These things are suggested in AN4068, but are not possible with out SKU
   //Cannot set front/back angle trip points in 0x13
   //Cannot set z lockout angle trip points in 0x13
   //Cannot set trip threshold angle in 0x14
   //Cannot set hysteresis angle in 0x14

   // enable interrupt detection and set which pins to route them to
   _SetEnabledInterrupts(INT_EN_LNDPRT, INT_EN_LNDPRT_INT1);

   // set debounce counter
   uint8_t debounce = 0x05;
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_PL_COUNT, 1, &debounce, 1, 1000);
   iprintf("Debounce Stat = 0x%x\n", stat);

   // enter active mode
   _SetEnterStandby(false);

   iprintf("Starting Orientation Loop...\n");
   while (1) {
      HAL_Delay(1000);

      // print port/land
      uint8_t regOrient;
      stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_PL_STATUS, 1, &regOrient, 1, 1000);
      if(stat != 0) {
         iprintf("PL Stat = 0x%x\n", stat);
         continue;
      }
      iprintf("Orient = 0x%x\n", regOrient);
   }

}

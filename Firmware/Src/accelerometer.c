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
#define        REG_SYSMOD        (0x0B)
#define        REG_INT_SOURCE    (0x0C)
#define        REG_WHO_AM_I      (0x0D)
#define        REG_XYZ_DATA_CFG  (0x0E)
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

static bool _ConfigureOrientationDetection();

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

   if(!_ConfigureOrientationDetection()) {
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
   INT_EN_FF_MT    = (0x1 << 2),
   INT_EN_LNDPRT   = (0x1 << 4),
   INT_EN_ASLP     = (0x1 << 7)
};

// this is a little weird. in each of the 8 positions (matching above), a 0 means int2, 1 is int1
enum InterruptLine {
   INT_EN_FF_MT_INT1    = (0x1 << 2),
   INT_EN_FF_MT_INT2    = (0x0 << 2),
   INT_EN_LNDPRT_INT1   = (0x1 << 4),
   INT_EN_LNDPRT_INT2   = (0x0 << 4),
};

// expects a mask of enabled interrupts
static bool _SetEnabledInterrupts(enum Interrupts enabledInterrupts, enum InterruptLine enabledLines) {
   HAL_StatusTypeDef stat;
   uint8_t reg = 0;

   //TODO enable wake from motion
   // interrupt polarity active high, push/pull, orientation can wake accele
   reg = (0x0 << 0) | (0x1 << 1) | (0x1 << 5);
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_CTRL3, 1, &reg, 1, 1000);
   iprintf("Standby 2 Stat = 0x%x\n", stat);

   stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_CTRL4, 1, &reg, 1, 1000);
   iprintf("CTRL4 1 Stat = 0x%x\n", stat);
   reg |= enabledInterrupts;
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_CTRL4, 1, &reg, 1, 1000);
   iprintf("CTRL4 2 Stat = 0x%x\n", stat);

   reg = enabledLines;
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_CTRL5, 1, &reg, 1, 1000);
   iprintf("Standby 2 Stat = 0x%x\n", stat);

   // True on success
   return (stat == 0);
}

static bool _ConfigureOrientationDetection() {
   HAL_StatusTypeDef stat;

   if(!_SetEnterStandby(true)) {
      return false;
   }

   //set sample rate to 50Hz
   if(!_SetSampleRate(SAMPLE_RATE_50HZ)) {
      return false;
   }

   // set full scale
   uint8_t regScale = 0x1 << 0;
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_XYZ_DATA_CFG, 1, &regScale, 1, 1000);
   if(stat != 0) {
      iprintf("Enable orientation detect stat = 0x%x\n", stat);
      return false;
   }

   //enable port/land mode 0x11
   uint8_t regPlCFG = (0x1 << 6) | (0x1 << 7);
   stat = HAL_I2C_Mem_Write(&hi2c1, ACCELE_ADDR, REG_PL_CFG, 1, &regPlCFG, 1, 1000);
   if(stat != 0) {
      iprintf("Enable orientation detect stat = 0x%x\n", stat);
      return false;
   }

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
   if(stat != 0) {
      iprintf("Debounce Stat = 0x%x\n", stat);
      return false;
   }

   // enter active mode
   _SetEnterStandby(false);

   return true;
}

void accelerometer_DecodeInterrupt(void) {
   HAL_StatusTypeDef stat;
   uint8_t regOrient;
   uint8_t int1, int2;

   // get int source
   stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_INT_SOURCE, 1, &regOrient, 1, 1000);
   if(stat != 0) {
      iprintf("int source Stat = 0x%x\n", stat);
   }
   iprintf("Int Source = 0x%x ", regOrient);

   // clear int and get orientation
   stat = HAL_I2C_Mem_Read(&hi2c1, ACCELE_ADDR, REG_PL_STATUS, 1, &regOrient, 1, 1000);
   if(stat != 0) {
      iprintf("PL Stat = 0x%x\n", stat);
   }
   iprintf("Orient (PL Status) = 0x%x ", regOrient);
}


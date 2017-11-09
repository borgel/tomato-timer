/*
 * The root of all ISRs for the entire program live in this file.
 */
#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include "stm32f0xx_it.h"
#include "stm32f0xx_hal_tim.h"
#include "stm32f0xx_hal_tim_ex.h"

#include "platform_hw.h"

#include "main.h"
#include "iprintf.h"

extern I2C_HandleTypeDef hi2c1;

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
   HAL_IncTick();
   HAL_SYSTICK_IRQHandler();
}

// Handle events on pins 0-1
void EXTI0_1_IRQHandler(void) {
   if(__HAL_GPIO_EXTI_GET_IT(nCHG_Pin)) {
      __HAL_GPIO_EXTI_CLEAR_IT(nCHG_Pin);

      iprintf("nCHG\n");

      //TODO something useful
   }
}

// Handle things on pins 4-15
void EXTI4_15_IRQHandler(void) {
   if(__HAL_GPIO_EXTI_GET_IT(ACCEL_INT1_Pin)) {
      __HAL_GPIO_EXTI_CLEAR_IT(ACCEL_INT1_Pin);
      iprintf("INT1\n");
   }
   if(__HAL_GPIO_EXTI_GET_IT(ACCEL_INT2_Pin)) {
      __HAL_GPIO_EXTI_CLEAR_IT(ACCEL_INT2_Pin);
      iprintf("INT2\n");
   }
}

/**
* @brief This function handles I2C1 event global interrupt / I2C1 wake-up interrupt through EXTI line 23.
*/
void I2C1_IRQHandler(void)
{
  if (hi2c1.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR)) {
    HAL_I2C_ER_IRQHandler(&hi2c1);
  } else {
    HAL_I2C_EV_IRQHandler(&hi2c1);
  }
}

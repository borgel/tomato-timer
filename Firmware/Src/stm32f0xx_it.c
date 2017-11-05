/*
 * The root of all ISRs for the entire program live in this file.
 */
#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include "stm32f0xx_it.h"
#include "stm32f0xx_hal_tim.h"
#include "stm32f0xx_hal_tim_ex.h"

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

/*
 * Handle any EXTI0 events (IE all GPIO Pin 0's)
 */
void EXTI0_1_IRQHandler(void) {
   if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0)) {
      __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);

      main_ButtonCB();
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

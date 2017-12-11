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
#include <stdbool.h>

#define     MAX_SESSION_LENGTH         (36)
#define     SESSION_LENGTH_LONG        (20)
#define     SESSION_LENGTH_SHORT       (5)
#define     SECONDS_TO_MINS            (60)

struct badTimer {
   uint32_t duration;
   uint32_t start;
   bool complete;
};

enum sessionState {
   SESSION_UNSTARTED,
   SESSION_IN_PROGRESS,
   SESSION_CLEANUP,
   SESSION_COMPLETE,       //from here done, count as coumpleted or disabled
   SESSION_CHARGING,
};

struct basicCountdown {
   uint8_t current;        // count UP from here until 36. number of LEDs is amount of time
   enum sessionState state;
};

union Interrupts {
   uint32_t mask;
   struct {
      uint8_t     accelerometer  : 1;
      uint8_t     charger        : 1;
   };
};

struct State {
   bool charging;
};
static struct State state = {0};

static union Interrupts ints;

static void _TimerSet(struct badTimer * t, uint32_t const duration);
static void _TimerReset(struct badTimer * t);
static void _TimerRestart(struct badTimer * t);
static bool _TimerHasElapsed(struct badTimer * const t);

static void _EnterDeepSleep(void);

int main(void)
{
   struct badTimer timer;
   //FIXME session unstarted?
   struct basicCountdown session = {.state = SESSION_COMPLETE};

   HAL_Init();

   platformHW_Init();

   iprintf("\r\nStarting... (v%d | #0x%x / 0x%x | Built "__DATE__":"__TIME__")\r\n", FW_VERSION, bid_GetID(), bid_GetIDCrc());

   _TimerReset(&timer);

   led_Init();
   accelerometer_Init();

   // boot "animation"
   for(int i = 0; i < 18; i++) {
      led_SetChannel(18 + i, 50);
      led_SetChannel(18 - i, 50);
      HAL_Delay(30);
   }
   led_ClearDisplay();

   struct accelerometer_CombinedOrientation orient;
   while(1) {
      // Process all pending interrupts
      while(ints.mask) {
         // assume any time this is set, there has been a change
         if(ints.charger) {
            // mark the timer as 'handled'
            ints.charger = 0;

            //FIXME rm
            iprintf("charging = %d\n", state.charging);

            if(state.charging) {
               session.state = SESSION_CHARGING;
               _TimerReset(&timer);

               //light middle LED when charging
               //TODO toggle periodically or something instead of solid on
               led_ClearDisplay();
               led_SetChannel(MAX_SESSION_LENGTH / 2, 80);
            }
            else {
               // not charging, re-enable system
               session.state = SESSION_UNSTARTED;
               led_ClearDisplay();

               //TODO de-configure accele interrupts instead of state stuff
            }
         }

         if(ints.accelerometer) {
            // mark the accelerometer as 'handled'
            ints.accelerometer = 0;

            // if we are charging, don't stare a new session
            if(session.state >=  SESSION_CHARGING) {
            }

            session.current = 0;

            orient = accelerometer_DecodeInterrupt();
            if(   orient.orient == AC_ORIENT_PORTRAIT_UP ||
                  orient.orient == AC_ORIENT_PORTRAIT_DOWN) {

               iprintf("Portrait session start\n");

               led_ClearDisplay();

               // configure this work session
               session.current = MAX_SESSION_LENGTH - SESSION_LENGTH_LONG;
               session.state = SESSION_IN_PROGRESS;

               // display the first light
               led_SetChannel(session.current, 80);

               // animation frame timer
               _TimerSet(&timer, 1000);
            }
            else if( orient.orient == AC_ORIENT_LANDSCAPE_RIGHT ||
                     orient.orient == AC_ORIENT_LANDSCAPE_LEFT) {

               iprintf("Landscape session start\n");

               led_ClearDisplay();

               // configure this work session
               session.current = MAX_SESSION_LENGTH - SESSION_LENGTH_SHORT;
               session.state = SESSION_IN_PROGRESS;

               // display the first light
               led_SetChannel(session.current, 80);

               // animation frame timer
               _TimerSet(&timer, 1000);
            }
            else if(orient.orient ==  AC_ORIENT_Z_LOCKOUT) {
               iprintf("Z Lockout - no gesture\n");

               led_ClearDisplay();
               _TimerReset(&timer);
               session.state = SESSION_COMPLETE;
            }
         }
      }

      if(   session.state < SESSION_COMPLETE &&
            _TimerHasElapsed(&timer)) {

         //FIXME rm
         iprintf("TIMER EXPIRED\n");

         switch(session.state) {
            case SESSION_IN_PROGRESS:
               if(session.current > 0) {
                  led_SetChannel(session.current - 1, 0);
               }
               led_SetChannel(session.current, 80);
               session.current++;

               if(session.current >= 36) {
                  // we're done
                  session.state = SESSION_CLEANUP;
               }

               // always restart this so we can cleanup LEDs
               _TimerRestart(&timer);
               break;

            case SESSION_CLEANUP:
               // end animation
               for(int i = 35; i >= 0; i--) {
                  led_SetChannel(i, 50);
                  HAL_Delay(10);
               }
               // disable all LEDs
               led_ClearDisplay();
               session.state = SESSION_COMPLETE;
               break;

            case SESSION_UNSTARTED:
            default:
               break;
         }
      }

      // if the we are just waiting, go to an intermediate power state.
      HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

      // if the device is idle, go to deep sleep
      if(session.state == SESSION_COMPLETE) {
         iprintf("Trying to deep sleep...\n");

         // this will return from reset
         _EnterDeepSleep();
      }
   }

   return 0;
}

static void _TimerReset(struct badTimer * t) {
   t->duration = 0;
   t->start = 0;
   t->complete = true;
}

static void _TimerSet(struct badTimer * t, uint32_t const duration) {
   //scale duration to minutes
   t->duration = duration * SECONDS_TO_MINS;
   _TimerRestart(t);
}

static void _TimerRestart(struct badTimer * t) {
   t->start = HAL_GetTick();
   t->complete = false;
}

static bool _TimerHasElapsed(struct badTimer * const t) {
   // only return EDGES
   if(t->complete) {
      return false;
   }

   //FIXME rm
   //iprintf("%d - %d = %d ( > %d)\n", HAL_GetTick(), t->start, (HAL_GetTick() - t->start), t->duration);

   if(HAL_GetTick() - t->start > t->duration) {
      t->complete = true;
      return true;
   }
   return false;
}

static void _EnterDeepSleep(void) {
   led_SetShutdown(true);

   //VREG LPR?

   //enter STOP. We will wake from any EXTI
   HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
   // reconfigure our clocks
   SystemClock_Config();

   //TODO enter STANDBY
   //TODO enable accele int 2 to wake up
   //HAL_PWR_EnableWakeUpPin(1);
   //HAL_PWR_EnterSTANDBYMode();

   led_SetShutdown(false);
}

void main_SetAcceleInt(void) {
   ints.accelerometer = 1;
}

void main_SetChargeState(bool chargeEn) {
   state.charging = chargeEn;
   ints.charger = 1;
}


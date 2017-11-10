#pragma once

#include <stdbool.h>
#include <stdint.h>

enum accelerometer_Orientation {
   AC_ORIENT_Z_LOCKOUT,    //back or front
   AC_ORIENT_PORTRAIT_UP,
   AC_ORIENT_PORTRAIT_DOWN,
   AC_ORIENT_LANDSCAPE_RIGHT,
   AC_ORIENT_LANDSCAPE_LEFT,
};
enum accelerometer_FrontBack {
   AC_FB_FRONT,
   AC_FB_BACK,
};

struct accelerometer_CombinedOrientation {
   enum accelerometer_Orientation   orient;
   enum accelerometer_FrontBack     frontback;

   uint32_t                         timeUS;
};

bool accelerometer_Init(void);
void accelerometer_TestStream(void);

struct accelerometer_CombinedOrientation accelerometer_DecodeInterrupt(void);


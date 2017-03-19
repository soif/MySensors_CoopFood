/*
DebugUtils.h - Simple debugging utilities.
Ideas taken from:
http://forum.arduino.cc/index.php?topic=46900.0
v2.0

usage:
#define OWN_DEBUG
#include "debug.h"

*/


#ifdef OWN_DEBUG
#include <Arduino.h>

#define DEBUG_PRINT(str)    \
   Serial.print(str);

#define DEBUG_PRINTDEC(str)	\
   Serial.print(str,DEC);

#define DEBUG_PRINTHEX(str)	\
   Serial.print(str,HEX);

#define DEBUG_PRINTLN(str) \
   Serial.println(str);

#else

#define DEBUG_PRINT(str)
#define DEBUG_PRINTDEC(str)
#define DEBUG_PRINTHEX(str)
#define DEBUG_PRINTLN(str)

#endif



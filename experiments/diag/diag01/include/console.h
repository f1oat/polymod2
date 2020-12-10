#pragma once

#include <Arduino.h>
#include <ArduinoSTL.h>

#include "board.h"

#define printf please_use_xprintf

#ifndef NO_FLASH_STRING
void xprintf(const __FlashStringHelper*, ...);
#else
void xprintf(const char*, ...);
#endif


extern void pollCLI();
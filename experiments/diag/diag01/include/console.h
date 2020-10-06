#pragma once

#include <Arduino.h>
#include <ArduinoSTL.h>

#define printf please_use_xprintf

void xprintf(const __FlashStringHelper*, ...);

extern void pollCLI();
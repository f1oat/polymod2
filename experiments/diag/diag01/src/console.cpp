#include <Arduino.h>
#include <ArduinoSTL.h>
#include "console.h"

void xprintf(const __FlashStringHelper* fmt, ...) {
  char buf[64];
  va_list args;
  va_start(args, fmt);
  vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args);
  va_end(args);
  Serial.print(buf);
}


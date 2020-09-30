#include <Arduino.h>
#include <ArduinoSTL.h>
#include "console.h"

void xprintf(std::string fmt, ...) {
  char buf[128];     // resulting string limited to 128 chars
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 128, fmt.c_str(), args);
  va_end(args);
  Serial.print(buf);
}


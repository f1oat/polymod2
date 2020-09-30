#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>
#include <ArduinoSTL.h>

#define printf please_use_xprintf

void xprintf(std::string fmt, ...);

#endif /*CONSOLE_H*/
#include <Arduino.h>
#include <ArduinoSTL.h>
#include <CLI.h>

#include "module.h"
#include "console.h"

void xprintf(const __FlashStringHelper* fmt, ...) {
  char buf[64];
  va_list args;
  va_start(args, fmt);
  vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args);
  va_end(args);
  Serial.print(buf);
}

char readEcho()
{
    while (!Serial.available());
    char c = Serial.read();
    Serial.write(c);
    return c;
}

String readToken()
{
    String result;
    bool done = false;

    while (!done) {
        char c = readEcho();
        switch (c) {
            case '\r':
                done = true;
                break;
            case ' ':
                if (result.length() > 0) done = true;
                break;
            default:
                result += c;
                break;
        }
    }

    return result;
}

String readLine()
{
    String result;
    bool done = false;

    while (!done) {
        char c = readEcho();
        switch (c) {
            case '\b':
                result.remove(result.length()-1);
                break;
            case '\r':
                done = true;
                break;
            default:
                result += c;
                break;
        }
    }

    return result;
}

static void help() {
    Serial.println(F("\nUsage:"));
    Serial.println(F("c: print current configuration"));
    Serial.println(F("w: write config to EEPROM"));
    Serial.println(F("ai <pin> <pin> ...: define analog inputs"));
    Serial.println(F("di <pin> <pin> ...: define digital inputs"));
    Serial.println(F("do <pin> <pin> ...: define digital outputs"));
    Serial.println(F("si <pin> <pin> ...: define socket inputs"));
    Serial.println(F("so <pin> <pin> ...: define socket outputs"));
}

static void definePins()
{
    String line = readLine();
    pinType_t pinType = undefined;
    
    switch (line[0]) {
        case 'a':
            pinType = analogInput;
            break;
        case 'd':
            pinType = (line[1] == 'i') ? digitalInput : digitalOutput;
            break;
        case 's':
            pinType = (line[1] == 'i') ? socketInput : socketOutput;
            break;
    }

    int rc = Module.parsePins(pinType, line.substring(2));
    if (rc) Serial.println(F("\nsyntax error"));
    else Serial.println(F("\nconfiguration updated"));
}

void pollCLI()
{
    if (!Serial.available()) return;

    switch (Serial.peek()) {
        case '\n':
            Serial.read();
            break;
        case 'd':
        case 'a':
        case 's':
            Serial.print(">");
            definePins();
            break;
        case 'v':
            Serial.read();
            Module.dumpValues();
            break;
        case 'c':
            Serial.read();
            Module.dumpConfig();
            break;
        case 'w':
            Serial.read();
            Serial.print(F("save config (y/n) ?"));
            if (readEcho() == 'y') {
                Module.saveConfig();
                Serial.println(F("\nconfig saved"));
            }
            else {
                Serial.println(F("\nabort"));
            }
            break;
        case '\r':
        default:
            Serial.read();
            help();
            break;
    }
}

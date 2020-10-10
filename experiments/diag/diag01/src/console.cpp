#include <Arduino.h>
#include <ArduinoSTL.h>
#include <CLI.h>
#include <Wire.h>

#include "module.h"
#include "console.h"
#include "sysinfo.h"

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
                break;
            case '\n':
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
    Serial.println(F("c:                  print current configuration"));
    Serial.println(F("v:                  print pins value"));
    Serial.println(F("i:                  print system informations"));
    Serial.println(F("R:                  restart module"));
    Serial.println(F("m <moduId>:         set Module ID"));
    Serial.println(F("ai <pin> <pin> ...: define analog inputs"));
    Serial.println(F("di <pin> <pin> ...: define digital inputs"));
    Serial.println(F("do <pin> <pin> ...: define digital outputs"));
    Serial.println(F("si <pin> <pin> ...: define socket inputs"));
    Serial.println(F("so <pin> <pin> ...: define socket outputs"));
    Serial.println(F("w:                  write config to EEPROM"));
}

static void definePins(String cmdline)
{
    pinType_t pinType = undefined;
    
    switch (cmdline[0]) {
        case 'a':
            pinType = analogInput;
            break;
        case 'd':
            pinType = (cmdline[1] == 'i') ? digitalInput : digitalOutput;
            break;
        case 's':
            pinType = (cmdline[1] == 'i') ? socketInput : socketOutput;
            break;
    }

    int rc = Module.parsePins(pinType, cmdline.substring(2));
    if (rc) Serial.println(F("\nsyntax error"));
    else Serial.println(F("\nconfiguration updated"));
}

static void setModuleId(String cmdline)
{
    uint8_t moduleId = cmdline.substring(1).toInt();

    xprintf(F("Set Module ID to %d (y/n) ?"), moduleId);
    if (readEcho() == 'y') {
        Module.setModuleId(moduleId);
        Serial.println(F("\nModule ID updated"));
    }
    else {
        Serial.println(F("\nAbort"));
    }
}

static void writeConfig()
{
    Serial.print(F("Save config (y/n) ?"));
    if (readEcho() == 'y') {
        Module.saveConfig();
        Serial.println(F("\nConfiguration saved"));
    }
    else {
        Serial.println(F("\nAbort"));
    }
}

static void restartModule()
{
    void(* resetFunc) (void) = 0; //declare reset function @ address 0

    Serial.print(F("Restart module (y/n) ?"));
    if (readEcho() == 'y') {
        Serial.print(F("Restarting module\n"));
        delay(500);
        Wire.end();
        cli();
        resetFunc();
    }
    else {
        Serial.println(F("\nAbort"));
    }
}

void pollCLI()
{
    if (!Serial.available()) return;
    String cmdline = readLine();

    switch (cmdline[0]) {
        case 'd':
        case 'a':
        case 's':
            definePins(cmdline);
            break;
        case 'v':
            Module.dumpValues();
            break;
        case 'm':
            setModuleId(cmdline);
            break;
        case 'c':
            Module.dumpConfig();
            break;
        case 'w':
            writeConfig();
            break;
        case 'i':
            sysinfo.dumpStats();
            break;
        case 'R':
            restartModule();
        default:
            help();
            break;
    }
}

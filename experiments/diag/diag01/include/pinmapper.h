#pragma once

#include <Arduino.h>
#include <ArduinoSTL.h>

#include <vector>
#include <map>
#include <iterator>

#include "pinhandler.h"

using namespace std;

typedef struct {
  uint8_t pinId;
  uint16_t newValue;
} valueChangeEvent_t;

typedef struct {
  uint8_t pinId;
  connection_t from;
} connectionChangeEvent_t;

typedef vector<valueChangeEvent_t> valueChangeList_t;
typedef vector<connectionChangeEvent_t> connectionChangeList_t;

class pinMapper_t {
  protected:
    pinType_t pinType; 
    void printPinType();
  public:
    typedef vector<pinHandler_t> pinTable_t;
    pinTable_t pinTable;

    pinMapper_t(pinType_t pinType) { this->pinType = pinType; }
    ~pinMapper_t();

    // Pins configuration
    void definePins(vector<uint8_t> pins);
    vector<uint8_t> getPins();              // return current pins configuration
    uint8_t getNbPins() { return pinTable.size(); };

    // Values methods
    valueChangeList_t getValueChangeList(uint8_t readerIndex = 0);  // readerIndeswx should be one for console
    connectionChangeList_t getConnectionChangeList(uint8_t readerIndex = 0);
    void readPins();                        // Read value of all pins

    // Debug functions
    void dumpPins(bool showValues);         // Dump value or arduinoPin of all pins to console
    void dumpChanges();                     // Dump only pins that has changed

    // Connections detection methods
    void serialOut(uint8_t bitNumber);
    void serialIn(uint8_t bitNumber);
};


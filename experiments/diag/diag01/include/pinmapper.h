#pragma once

#include <Arduino.h>
#include <ArduinoSTL.h>

#include <vector>
#include <map>
#include <iterator>

#include "pinhandler.h"
#include "bitArray.h"

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
    bitArray *pinChange[2]; // bit field to track pin state change, 2 tables, one for I2C and one for console

    pinMapper_t(pinType_t pinType) { this->pinType = pinType; this->pinChange[0] = this->pinChange[1] = 0; }
    ~pinMapper_t();

    // Pins configuration
    void definePins(vector<uint8_t> pins);
    vector<uint8_t> getPins();              // return current pins configuration
    uint8_t getNbPins() { return pinTable.size(); };

    // Methods to get changed values and connections
    valueChangeList_t getValueChangeList(uint8_t readerIndex = 0);  // readerIndeswx should be one for console
    connectionChangeList_t getConnectionChangeList(uint8_t readerIndex = 0);
    
    bool getNextValueChange(valueChangeEvent_t &event, uint8_t readerIndex = 0);
    bool getNextConnectionChange(connectionChangeEvent_t &event, uint8_t readerIndex = 0);

    // Debug functions
    void dumpPins(bool showValues);         // Dump value or arduinoPin of all pins to console
    void dumpChanges();                     // Dump only pins that has changed

    // Input management methods
    void updateAllPins();                   // Read value of all pins and update pinChange bitfields if change

    // Connections detection methods
    void serialOut(uint8_t bitNumber);
    void serialIn(uint8_t bitNumber);
};


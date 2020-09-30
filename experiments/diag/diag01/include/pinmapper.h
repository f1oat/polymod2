#ifndef PINMAPPER_H
#define PINMAPPER_H

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
    uint8_t moduleId;

    typedef std::map<pinType_t, string> asciiLabels_t;
    static asciiLabels_t asciiLabels;

  public:
    vector<pinHandler_t*> pinTable;

    pinMapper_t(pinType_t pinType, uint8_t moduleId) : pinType(pinType), moduleId(moduleId) {};
    
    void dumpPins();                        // Dump value of all pins to console
    void dumpChanges();                     // Dump only pins that has changed

    void readPins();                        // Read value of all pins
    void definePins(vector<uint8_t> pins);  // define and connect pins to Arduinon physical pins

    void serialOut(uint8_t bitNumber);
    void serialIn(uint8_t bitNumber);

    uint8_t getNbPins() { return pinTable.size(); };

    valueChangeList_t getValueChangeList();
    connectionChangeList_t getConnectionChangeList();

};

#endif /*PINMAPPER_H*/

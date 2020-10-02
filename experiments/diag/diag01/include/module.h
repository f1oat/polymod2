#ifndef MODULE_H
#define MODULE_H

#include <Arduino.h>
#include <ArduinoSTL.h>

#include <vector>
#include <map>
#include <iterator>

#include "pinmapper.h"

using namespace std;

class Module_t {
  public:
    
  private:
    typedef std::map<pinType_t, pinMapper_t *> mapTable_t;
    mapTable_t mapTable;
    uint8_t moduleId;
    
  public:
    Module_t(uint8_t moduleId);

    // Mapping definition

    void definePins(pinType_t t, vector<uint8_t> pins);
    
    // I/O methods

    void updateAll();
    void stepConnections(uint8_t stepNumber);
    void detectConnections();

    uint16_t getValue(pinType_t t, int id) { return mapTable[t]->pinTable[id]->getValue();  };
    void setValue(pinType_t t, int id, uint16_t value) { mapTable[t]->pinTable[id]->setBitValue(value); };

    // get changes

    connectionChangeList_t getConnectionChangeList();
    valueChangeList_t getAnalogInputChangeList();
    valueChangeList_t getDigitalInputChangeList();

    // Debug methods
    
    void dumpValues();
    void dumpChanges();
};

#endif /*MODULE_H*/
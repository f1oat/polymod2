#pragma once

#include <Arduino.h>
#include <ArduinoSTL.h>

#include <vector>
#include <map>
#include <iterator>

#include "pinmapper.h"
#include "nvmem.h"

using namespace std;

class ModuleClass {
  public:
    
  private:
    typedef std::map<pinType_t, pinMapper_t *> mapTable_t;
    mapTable_t mapTable;
    uint8_t moduleId;
    
    void dumpPins(boolean showValues);

  public:
    ModuleClass();
    ~ModuleClass();

    // Module configuration

    void inline setModuleId(uint8_t moduleId) { this->moduleId = moduleId; };
    uint8_t inline getModuleId() { return moduleId; };

    void definePins(pinType_t t, vector<uint8_t> pins);
    int parsePins(pinType_t t, String);
    vector<byte> getPins(pinType_t t);

    void dumpConfig();

    // EEPROM save/load

    void saveConfig();
    bool loadConfig();
    bool checkConfig() { return NVMEM.CheckCRC(); };
    
    // I/O methods

    void updateAll();
    void stepConnections(uint8_t stepNumber);
    void detectConnections();

    uint16_t getValue(pinType_t t, int id) { return mapTable[t]->pinTable[id].getValue(); };
    void setValue(pinType_t t, int id, uint16_t value) { mapTable[t]->pinTable[id].setBitValue(value); };
    uint8_t getNbPins(pinType_t t) { return mapTable[t] ? mapTable[t]->pinTable.size() : 0; }

    // get changes

    connectionChangeList_t getConnectionChangeList();
    valueChangeList_t getAnalogInputChangeList();
    valueChangeList_t getDigitalInputChangeList();

    // Debug methods
    
    void dumpValues();
    void dumpChanges();
    void blinkDigitalOutputs();
};

extern ModuleClass Module;
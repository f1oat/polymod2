#include <assert.h>

#include "console.h"
#include "module.h"
#include "nvmem.h"

ModuleClass Module;

ModuleClass::ModuleClass()
{
  vector<pinType_t> pinTypeList = { analogInput, digitalInput, digitalOutput, socketInput, socketOutput };
  
  for (uint8_t i = 0; i < pinTypeList.size(); i++) {
    mapTable[pinTypeList[i]] = new pinMapper_t(pinTypeList[i]);
  }
}

ModuleClass::~ModuleClass()
{
  mapTable_t::iterator it;
  for (it = mapTable.begin(); it != mapTable.end(); it++) {
    delete it->second;
  }
}

void ModuleClass::saveConfig()
{
  mapTable_t::iterator it;

  NVMEM.rewind();

  NVMEM.put(moduleId);
  NVMEM.put((uint8_t)mapTable.size()); // write number of pin types

  for (it = mapTable.begin(); it != mapTable.end(); it++) {
    vector<uint8_t> pins = it->second->getPins();
    NVMEM.put((uint8_t)it->first);                                      // write pin type
    NVMEM.put((uint8_t)pins.size());                           // write number of pins
    for (uint8_t i=0; i<pins.size(); i++) NVMEM.put(pins[i]);  // write each pin
  }

  NVMEM.UpdateCRC(); // Stamp EEPROM content with consistency check
}

bool ModuleClass::loadConfig()
{
  if (!checkConfig()) return false; // EEPROM content is invalid

  NVMEM.rewind();

  NVMEM.get(moduleId);

  uint8_t nbTypes;
  NVMEM.get(nbTypes);  // read number of pin types

  while (nbTypes-- > 0) {
    uint8_t pinType;
    uint8_t nbPins;
    
    NVMEM.get(pinType);                                        // read pin type
    NVMEM.get(nbPins);                                         // read number of pins
    
    vector<uint8_t> pins(nbPins);
    for (uint8_t i=0; i<nbPins; i++) {
      uint8_t pinArduino;
      NVMEM.get(pinArduino);  // read each pin
      pins[i] = pinArduino;
    }
    definePins((pinType_t)pinType, pins);
  }

  return true;
}

void ModuleClass::definePins(pinType_t t, vector<byte> pins)
{
  assert(mapTable[t]);
  mapTable[t]->definePins(pins);
}

vector<byte> ModuleClass::getPins(pinType_t t)
{
  return (mapTable[t]) ? mapTable[t]->getPins() : vector<byte>{};
}

void ModuleClass::updateAll()
{
  mapTable_t::iterator it;
  for (it = mapTable.begin(); it != mapTable.end(); it++) {
    it->second->readPins();
  }
}

void ModuleClass::blinkDigitalOutputs()
{
  for (uint8_t i=0; i<getNbPins(digitalOutput); i++) {
    setValue(digitalOutput, i, 1 - getValue(digitalOutput, i));
  }
}

void ModuleClass::dumpValues()
{
  mapTable_t::iterator it;
  uint8_t maxNbPins = 0;

  for (it = mapTable.begin(); it != mapTable.end(); it++) {
    maxNbPins = max(it->second->getNbPins(), maxNbPins);
  } 

  xprintf(F("-----------------------------------------\n"));
  xprintf(F("%15s"), "");
  for (int i = 0; i < maxNbPins; i++) xprintf(F("   %02d"), i);
  xprintf(F("\n"));
  for (it = mapTable.begin(); it != mapTable.end(); it++) it->second->dumpPins();
  xprintf(F("-----------------------------------------\n"));
}

void ModuleClass::dumpChanges()
{
  mapTable[analogInput]->dumpChanges();
  mapTable[digitalInput]->dumpChanges();
  mapTable[socketInput]->dumpChanges();
}

connectionChangeList_t ModuleClass::getConnectionChangeList()
{
  connectionChangeList_t list = mapTable[socketInput]->getConnectionChangeList();
  return list;
}

valueChangeList_t ModuleClass::getAnalogInputChangeList()
{
  valueChangeList_t list = mapTable[analogInput]->getValueChangeList();
  return list;
}

valueChangeList_t ModuleClass::getDigitalInputChangeList()
{
  valueChangeList_t list = mapTable[digitalInput]->getValueChangeList();
  return list;
}

void ModuleClass::stepConnections(uint8_t stepNumber)
{
  uint8_t bitNumber = stepNumber >> 1;
  if ( (stepNumber & 1) == 0) mapTable[socketOutput]->serialOut(bitNumber);
  else mapTable[socketInput]->serialIn(bitNumber);
}

void ModuleClass::detectConnections()
{
  for (uint8_t stepNumber = 0; stepNumber < 32; stepNumber++) stepConnections(stepNumber);
}

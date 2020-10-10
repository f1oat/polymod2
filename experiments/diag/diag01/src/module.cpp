#include <assert.h>

#include "console.h"
#include "module.h"
#include "nvmem.h"

ModuleClass Module;

ModuleClass::ModuleClass()
{
  vector<pinType_t> pinTypeList = {analogInput, digitalInput, digitalOutput, socketInput, socketOutput};

  for (uint8_t i = 0; i < pinTypeList.size(); i++)
  {
    mapTable[pinTypeList[i]] = new pinMapper_t(pinTypeList[i]);
  }
}

ModuleClass::~ModuleClass()
{
  mapTable_t::iterator it;
  for (it = mapTable.begin(); it != mapTable.end(); it++)
  {
    delete it->second;
  }
}

// Config is now stored in TLV format

typedef enum
{
  tagModuleId = 0,
  tagPin = 1,
  tagEnd = 0xFF
} nvramTag_t;

#define PUT_TL(T, L)         \
  {                          \
    NVMEM.put((uint8_t)(T));            \
    NVMEM.put((uint8_t)(L)); \
  }

void ModuleClass::saveConfig()
{
  mapTable_t::iterator it;

  NVMEM.rewind();

  // Write moduleId

  PUT_TL(tagModuleId, 1);
  NVMEM.put(moduleId);

  // Write config for each pin

  for (it = mapTable.begin(); it != mapTable.end(); it++)
  {
    vector<uint8_t> pins = it->second->getPins();
    PUT_TL(tagPin, 1 + pins.size()); // write head for TLV
    NVMEM.put((uint8_t)it->first);   // write pin type
    for (uint8_t i = 0; i < pins.size(); i++)
      NVMEM.put(pins[i]); // write each pin
  }

  PUT_TL(tagEnd, 0);
  NVMEM.UpdateCRC(); // Stamp EEPROM content with consistency check
}

bool ModuleClass::loadConfig()
{
  bool end = false;

  if (!checkConfig())
    return false; // EEPROM content is invalid

  NVMEM.rewind();

  while (!end && !NVMEM.eof())
  {
    uint8_t tag;
    uint8_t len;
    NVMEM.get(tag);
    NVMEM.get(len);

    //xprintf(F("tag=%d len=%d\n"), tag, len);

    switch (tag)
    {
    case tagModuleId:
      NVMEM.get(moduleId);
      break;

    case tagPin:
      {
        uint8_t pinType;
        NVMEM.get(pinType); // read pin type
        vector<uint8_t> pins(len-1);
        for (uint8_t i = 0; i < len-1; i++)
        {
          uint8_t pinArduino;
          NVMEM.get(pinArduino); // read each pin
          pins[i] = pinArduino;
        }
        definePins((pinType_t)pinType, pins);
      }
      break;

    case tagEnd:
      end = true;
      break;

    default:
      xprintf(F("Skipping unkown tag %d\n"), tag);
      for (uint8_t i = 0; i < len; i++)
      {
        uint8_t dummy;
        NVMEM.get(dummy);
      }
      end = true;
      break;
    }
  }

  return true;
}

void ModuleClass::definePins(pinType_t t, vector<byte> pins)
{
  assert(mapTable[t]);
  mapTable[t]->definePins(pins);
}

int ModuleClass::parsePins(pinType_t t, String list)
{
  //xprintf(F("definePins %d %s\n"), t, list.c_str());

  if (t == undefined)
    return -1;

  // Parse the list of pins, separated by spaces

  vector<uint8_t> pins;

  uint8_t b = 0;
  uint8_t e = 1;
  while (b < list.length())
  {
    if (list[b] == ' ')
      b++;
    else
    {
      e = b;
      while (e < list.length() && list[e] != ' ')
        e++;
      String s = list.substring(b, e);
      uint8_t newPin = (s[0] == 'A') ? s.substring(1).toInt() + A0 : s.toInt();
      pins.push_back(newPin);
      b = e + 1;
    }
  }

  mapTable[t]->definePins(pins);
  return 0;
}

vector<byte> ModuleClass::getPins(pinType_t t)
{
  return (mapTable[t]) ? mapTable[t]->getPins() : vector<byte>{};
}

void ModuleClass::updateAll()
{
  mapTable_t::iterator it;
  for (it = mapTable.begin(); it != mapTable.end(); it++)
  {
    it->second->readPins();
  }
}

void ModuleClass::blinkDigitalOutputs()
{
  for (uint8_t i = 0; i < getNbPins(digitalOutput); i++)
  {
    setValue(digitalOutput, i, 1 - getValue(digitalOutput, i));
  }
}

void ModuleClass::dumpPins(boolean showValues)
{
  mapTable_t::iterator it;
  uint8_t maxNbPins = 0;

  for (it = mapTable.begin(); it != mapTable.end(); it++)
  {
    maxNbPins = max(it->second->getNbPins(), maxNbPins);
  }

  Serial.println();
  xprintf(showValues ? F("----VALUES-----") : F("-PHYSICAL PINS-"));

  for (int i = 0; i < maxNbPins; i++)
    xprintf(F("   %02d"), i);
  Serial.println();

  for (it = mapTable.begin(); it != mapTable.end(); it++)
  {
    it->second->dumpPins(showValues);
  }
  Serial.println();
}

void ModuleClass::dumpConfig()
{
  xprintf(F("=== Module ID = %d ==="), Module.getModuleId());
  dumpPins(false);
}

void ModuleClass::dumpValues()
{
  dumpPins(true);
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
  if ((stepNumber & 1) == 0)
    mapTable[socketOutput]->serialOut(bitNumber);
  else
    mapTable[socketInput]->serialIn(bitNumber);
}

void ModuleClass::detectConnections()
{
  for (uint8_t stepNumber = 0; stepNumber < 32; stepNumber++)
    stepConnections(stepNumber);
}

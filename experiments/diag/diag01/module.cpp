#include "module.h"

Module_t::Module_t(uint8_t moduleId)
{
  this->moduleId = moduleId;
  vector<pinType_t> list = { analogInput, digitalInput, digitalOutput, socketInput, socketOutput };
  for (int i = 0; i < list.size(); i++) {
    mapTable[list[i]] = new pinMapper_t(list[i], moduleId);
  }
}

void Module_t::definePins(pinType_t t, vector<byte> pins)
{
  mapTable[t]->definePins(pins);
}

void Module_t::updateAll()
{
  mapTable_t::iterator it;

  for (it = mapTable.begin(); it != mapTable.end(); it++) {
    it->second->readPins();
  }
}

void Module_t::dumpValues()
{
  mapTable_t::iterator it;
  uint8_t maxNbPins = 0;

  for (it = mapTable.begin(); it != mapTable.end(); it++) {
    maxNbPins = max(it->second->getNbPins(), maxNbPins);
  }

  printf("-----------------------------------------\n");
  printf("%15s", "");
  for (int i = 0; i < maxNbPins; i++) printf("   %02d", i);
  printf("\n");
  for (it = mapTable.begin(); it != mapTable.end(); it++) it->second->dumpPins();
  printf("-----------------------------------------\n");
}

void Module_t::dumpChanges()
{
  mapTable[analogInput]->dumpChanges();
  mapTable[digitalInput]->dumpChanges();
  mapTable[socketInput]->dumpChanges();
}

void Module_t::stepConnections(uint8_t stepNumber)
{
  uint8_t bitNumber = stepNumber >> 1;
  if ( (stepNumber & 1) == 0) mapTable[socketOutput]->serialOut(bitNumber);
  else mapTable[socketInput]->serialIn(bitNumber);
}


void Module_t::detectConnections()
{
  for (uint8_t stepNumber = 0; stepNumber < 32; stepNumber++) stepConnections(stepNumber);
}
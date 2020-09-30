#include "module.h"
#include "console.h"

pinMapper_t::asciiLabels_t pinMapper_t::asciiLabels = {
      { analogInput,    "Analog Input" },
      { digitalInput,   "DigitalInput" },
      { digitalOutput,  "Digital Output" }, 
      { socketInput,    "Socket Input" },
      { socketOutput,   "Socket Output" },
    };

void pinMapper_t::definePins(vector<uint8_t> pins)
{
  uint8_t pinId;
  for (pinId = 0; pinId < pins.size(); pinId++) {
    pinTable.push_back(new pinHandler_t(pinType, pins[pinId], moduleId, pinId));
  }
}

void pinMapper_t::readPins()
{  
  vector<pinHandler_t *>::iterator pin;
  for (pin = pinTable.begin(); pin < pinTable.end(); pin++) (*pin)->updateValue();
}

valueChangeList_t pinMapper_t::getValueChangeList()
{
  valueChangeEvent_t ev;
  valueChangeList_t list;

  for (ev.pinId = 0; ev.pinId < pinTable.size(); ev.pinId++) {
    bool changed;
    ev.newValue = pinTable[ev.pinId]->getValue(&changed);
    if (changed) list.push_back(ev);
  }

  return list;
}

connectionChangeList_t pinMapper_t::getConnectionChangeList()
{
  connectionChangeEvent_t ev;
  connectionChangeList_t list;

  for (ev.pinId = 0; ev.pinId < pinTable.size(); ev.pinId++) {
    bool changed;
    ev.from = pinTable[ev.pinId]->getConnection(&changed);
    if (changed) list.push_back(ev);
  }

  return list;
}

void pinMapper_t::serialOut(uint8_t bitNumber)
{
  vector<pinHandler_t*>::iterator pin;
  for (pin = pinTable.begin(); pin < pinTable.end(); pin++) {
    (*pin)->serialOut(bitNumber);
  }
}

void pinMapper_t::serialIn(uint8_t bitNumber)
{
  vector<pinHandler_t*>::iterator pin;
  for (pin = pinTable.begin(); pin < pinTable.end(); pin++) {
    (*pin)->serialIn(bitNumber);
  }
}

void pinMapper_t::dumpPins()
{
  xprintf("%15s", asciiLabels[pinType].c_str());
  for (uint8_t id = 0; id < pinTable.size(); id++) xprintf("%5d", pinTable[id]->getValue());
  xprintf("\n");
}

void pinMapper_t::dumpChanges()
{
  if (pinType != socketInput) {
    valueChangeList_t list = getValueChangeList();
    if (list.size() == 0) return;
    for (uint8_t i = 0; i < list.size(); i++) xprintf("%-14s [%02d] = %5d\n", asciiLabels[pinType].c_str(), list[i].pinId, list[i].newValue);
  }
  else {
    connectionChangeList_t list = getConnectionChangeList();
    if (list.size() == 0) return;
    for (uint8_t i = 0; i < list.size(); i++) {
      string label = list[i].from.isConnected ? "Connection" : "Disconnection";
      xprintf("%-14s [%02d.%02d] -> [%02d.%02d]\n", label.c_str(), list[i].from.moduleId, list[i].from.pinId, this->moduleId, list[i].pinId);
    }
  }
}


#include "module.h"
#include "console.h"

void pinMapper_t::definePins(vector<uint8_t> pins)
{
  this->pinType = pinType;

  pinTable.clear();
  if (pinChange[0]) delete pinChange[0];
  if (pinChange[1]) delete pinChange[1];
  
  for (uint8_t pinId = 0; pinId < pins.size(); pinId++) {
    pinHandler_t pinHandler(pinType, pins[pinId], pinId);
    pinTable.push_back(pinHandler);
  }

  pinChange[0] = new bitArray(pins.size());
  pinChange[1] = new bitArray(pins.size()); 
}

pinMapper_t::~pinMapper_t()
{
  /*
  pinTable_t::iterator pin;
  for (pin = pinTable.begin(); pin < pinTable.end(); pin++) {
    delete (*pin);
  }
  */
}

vector<uint8_t> pinMapper_t::getPins()
{
  vector<uint8_t> pins;

  pinTable_t::iterator pin;
  for (pin = pinTable.begin(); pin < pinTable.end(); pin++) {
    pins.push_back((*pin).getPinArduino());
  }

  return pins;
}

void pinMapper_t::readPins()
{  
  for (uint8_t i = 0; i < pinTable.size(); i++) {
    if (pinTable[i].updateValue()) {   // pin change
      pinChange[0]->set(i, true);
      pinChange[1]->set(i, true);
    }
  }
}

valueChangeList_t pinMapper_t::getValueChangeList(uint8_t readerIndex)
{
  valueChangeList_t list;

  for (uint8_t pinId = 0; pinId < pinTable.size(); pinId++) {
    if (pinChange[readerIndex]->get(pinId)) {
      list.push_back( { pinId, pinTable[pinId].getValue() });
      pinChange[readerIndex]->set(pinId, false);
    }
  }

  return list;
}

connectionChangeList_t pinMapper_t::getConnectionChangeList(uint8_t readerIndex)
{
  connectionChangeList_t list;

  for (uint8_t pinId = 0; pinId < pinTable.size(); pinId++) {
    if (pinChange[readerIndex]->get(pinId)) {
      list.push_back( { pinId, pinTable[pinId].getConnection() });
      pinChange[readerIndex]->set(pinId, false);
    }
  }

  return list;
}

void pinMapper_t::serialOut(uint8_t bitNumber)
{
  pinTable_t::iterator pin;
  for (pin = pinTable.begin(); pin < pinTable.end(); pin++) {
    (*pin).serialOut(bitNumber);
  }
}

void pinMapper_t::serialIn(uint8_t bitNumber)
{
  for (uint8_t i = 0; i < pinTable.size(); i++) {
    if (pinTable[i].serialIn(bitNumber)) {
      pinChange[0]->set(i, true);
      pinChange[1]->set(i, true);      
    }
  }
}

void pinMapper_t::printPinType()
{
    switch (pinType) {
    case analogInput:    Serial.print(F("   Analog Input")); break;
    case digitalInput:   Serial.print(F("  Digital Input")); break;
    case digitalOutput:  Serial.print(F(" Digital Output")); break;
    case socketInput:    Serial.print(F("   Socket Input")); break;
    case socketOutput:   Serial.print(F("  Socket Output")); break;
    default:             Serial.print(F("              ?")); break;
  }
}

void pinMapper_t::dumpPins(bool showValues)
{
  printPinType();

  for (uint8_t id = 0; id < pinTable.size(); id++) {
    if (showValues) xprintf(F("%5d"), pinTable[id].getValue());
    else xprintf(F("%5s"), pinTable[id].stringPinArduino().c_str());
  }
  xprintf(F("\n"));
}

void pinMapper_t::dumpChanges()
{
  if (pinType != socketInput) {
    valueChangeList_t list = getValueChangeList(1); // readerIndex = 1 means console
    if (list.size() == 0) return;
    for (uint8_t i = 0; i < list.size(); i++) {
      printPinType();
      xprintf(F("[%02d] = %5d\n"), list[i].pinId, list[i].newValue);
    }
  }
  else {
    connectionChangeList_t list = getConnectionChangeList(1); // readerIndex = 1 means console
    if (list.size() == 0) return;
    for (uint8_t i = 0; i < list.size(); i++) {
      string label = list[i].from.isConnected ? "Connection" : "Disconnection";
      xprintf(F("%-14s [%02d.%02d] -> [%02d.%02d]\n"), label.c_str(), list[i].from.moduleId, list[i].from.pinId, Module.getModuleId(), list[i].pinId);
    }
  }
}


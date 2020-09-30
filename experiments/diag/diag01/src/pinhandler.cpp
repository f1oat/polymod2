#include <assert.h>
#include "pinhandler.h"
#include "console.h"

pinHandler_t::pinHandler_t(pinType_t pinType, uint8_t pinArduino, uint8_t moduleId, uint8_t pinId)
{
  this->pinType = pinType;
  this->pinArduino = pinArduino;
  this->connection.setId(moduleId, pinId);

  switch (pinType) {
  case digitalInput:
  case socketInput:
    pinMode(pinArduino, INPUT_PULLUP);
    value.currentValue = digitalRead(pinArduino);
    break;
  case digitalOutput:
  case socketOutput:
    assert(pinArduino < A0);  // Avoid using an analog pin as output
    pinMode(pinArduino, OUTPUT);
    digitalWrite(pinArduino, 0);
    break;
  case analogInput:
    value.currentValue = analogRead(pinArduino) << 2;
    break;
  default:
    break;
  }

  value.prevValue = value.currentValue;
}

void pinHandler_t::updateValue()
{
  int threshold = 1;

  if (debounceCounter > 0) {
    debounceCounter--;
    return;
  }

  switch (pinType) {
  case digitalInput:
  case socketInput:
    value.currentValue = digitalRead(pinArduino);
    if (value.prevValue != value.currentValue) debounceCounter = debounceDelay;
    break;
  case digitalOutput:
  case socketOutput:
    value.currentValue = digitalRead(pinArduino);
    break;
  case analogInput:
    // Apply IIR filter with coeff 0.25
    value.currentValue -= value.currentValue >> 2;
    value.currentValue += analogRead(pinArduino);
    threshold = 4;
    break;
  default:
    break;
  }

  int delta = abs((int)value.prevValue - (int)value.currentValue);

  if (delta >= threshold) {
    value.prevValue = value.currentValue;
    value.changed = true;
  }
}

void pinHandler_t::setBitValue(uint8_t value)
{
  switch (pinType) {
  case digitalOutput:
  case socketOutput:
    digitalWrite(pinArduino, value);
    break;
  case digitalInput:
  case socketInput:
  case analogInput:
    break;
  default:
    break;
  }
}

uint16_t pinHandler_t::getValue(bool *hasChanged)
{
  if (hasChanged) {
    *hasChanged = value.changed;
    value.changed = false;
  }

  return value.currentValue;
}

connection_t pinHandler_t::getConnection(bool* hasChanged)
{
  if (hasChanged) {
    *hasChanged = connection.changed;
    connection.changed = false;
  }

  return connection.getConnection();
}

void pinHandler_t::serialOut(uint16_t bitNumber)
{
  digitalWrite(pinArduino, (connection.serialBuffer >> bitNumber) & 1);
}

void pinHandler_t::serialIn(uint16_t bitNumber)
{
  uint16_t bit = digitalRead(pinArduino);
  if (bitNumber == 0) connection.serialBuffer = 0;
  connection.serialBuffer |= bit << bitNumber;

  if (bitNumber != 15) return; // 16 bits not yet received

  if (connection.prevSerialBuffer != connection.serialBuffer) { // Received ID not confirmed, probably glitch during cable connection
    connection.prevSerialBuffer = connection.serialBuffer;
    return; 
  }

  if (connection.isConnected && connection.serialBuffer == 0xFFFF) {  // Disconnection
    connection.isConnected = false;
    connection.changed = true;
  }
  else if (!connection.isConnected && connection.serialBuffer != 0xFFFF) {  // New connection
    connection.confirmedConnection = connection.serialBuffer;
    connection.isConnected = true;
    connection.changed = true;
  }
}
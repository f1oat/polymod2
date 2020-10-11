#include <assert.h>

#include "pinhandler.h"
#include "module.h"
#include "console.h"

uint8_t pinHandler_t::debounceDelay = 5;      // Delay before successive reads of digital input for debouncing
uint8_t pinHandler_t::denoiseFilterCoeff = 2; // Anlog filter constant will 1<<analogFilterCoeff 
uint8_t pinHandler_t::denoiseThreshold = 6;   // Threshold for change detection of analog inputs

pinHandler_t::pinHandler_t(pinType_t pinType, uint8_t pinArduino, uint8_t pinId)
{
  this->pinType = pinType;
  this->pinId = pinId;
  this->pinArduino = pinArduino;
  this->connection.setId(pinId);

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
    value.currentValue = analogRead(pinArduino) << denoiseFilterCoeff;
    break;
  default:
    break;
  }

  value.prevValue = value.currentValue;
}

void pinHandler_t::updateValue()
{
  if (debounceCounter > 0) {
    debounceCounter--;
    return;
  }

  switch (pinType) {
  case digitalInput:
  case socketInput:
    value.currentValue = digitalRead(pinArduino);
    if (value.prevValue != value.currentValue) {
      value.prevValue = value.currentValue;
      value.changed[0] = true;
      value.changed[1] = true;
      debounceCounter = debounceDelay;
    }
    break;
  case digitalOutput:
  case socketOutput:
    value.currentValue = digitalRead(pinArduino);
    break;
  case analogInput:
    {
      // Apply IIR filter with coeff 0.25
      value.currentValue -= value.currentValue >> denoiseFilterCoeff;
      value.currentValue += analogRead(pinArduino);
      int delta = abs((int)value.prevValue - (int)value.currentValue);
      if (delta > denoiseThreshold) {
        value.prevValue = value.currentValue;
        value.changed[0] = true;
        value.changed[1] = true;
      }
    }
    break;
  default:
    break;
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

uint16_t pinHandler_t::getValue(bool *hasChanged, uint8_t readerIndex)
{
  if (hasChanged && readerIndex < 2) {
    *hasChanged = value.changed[readerIndex];
    value.changed[readerIndex] = false;
  }

  if (pinType == analogInput) return value.currentValue >> denoiseFilterCoeff;
  else return value.currentValue;
}

connection_t pinHandler_t::getConnection(bool* hasChanged, uint8_t readerIndex)
{
  if (hasChanged) {
    *hasChanged = connection.changed[readerIndex];
    connection.changed[readerIndex] = false;
  }

  return connection.getConnection();
}

void pinHandler_t::serialOut(uint16_t bitNumber)
{
  if (bitNumber == 0) connection.serialBuffer = (Module.getModuleId() << 8) | pinId;
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
    connection.changed[0] = true;
    connection.changed[1] = true;
  }
  else if (!connection.isConnected && connection.serialBuffer != 0xFFFF) {  // New connection
    connection.confirmedConnection = connection.serialBuffer;
    connection.isConnected = true;
    connection.changed[0] = true;
    connection.changed[1] = true;
  }
}
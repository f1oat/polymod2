#include <assert.h>

#include "pinhandler.h"
#include "module.h"
#include "console.h"

uint8_t pinHandler_t::debounceDelay = 5;      // Delay before successive reads of digital input for debouncing
uint8_t pinHandler_t::denoiseFilterCoeff = 2; // Actual anolog IIR filter constant will be 1<<analogFilterCoeff 
uint8_t pinHandler_t::denoiseThreshold = 6;   // Threshold for change detection of analog inputs

pinHandler_t::pinHandler_t(pinType_t pinType, uint8_t pinArduino, uint8_t pinId)
{
  this->pinType = pinType;
  this->pinId = pinId;
  this->pinArduino = pinArduino;
  this->connection.setId(pinId);

  #ifdef ATMEGA_4809
  bool isAnalog = (pinArduino >= PIN_PD0 &&  pinArduino <= PIN_PE3) || (pinArduino >= PIN_PF2 &&  pinArduino <= PIN_PF5);
  #else
  bool isAnalog = (pinArduino >= A0);
  #endif

  switch (pinType) {
  case digitalInput:
  case socketInput:
    pinMode(pinArduino, INPUT_PULLUP);
    value.currentValue = digitalRead(pinArduino);
    break;
  case digitalOutput:
  case socketOutput:
    // Avoid using an analog pin as output
    if (isAnalog) {
      xprintf(F("pin %d: cannot configured as a digital output\n"), pinArduino);
      break;
    }
    pinMode(pinArduino, OUTPUT);
    digitalWrite(pinArduino, 0);
    break;
  case analogInput:
    value.currentValue = analogRead(pinArduino) << denoiseFilterCoeff;
    break;
  case pwmOutput:
    if (!digitalPinHasPWM(pinArduino)) {
      Serial.println(F("Invalid PWM pin"));
      break;  
    }
    value.currentValue = 0;
    pinMode(pinArduino, OUTPUT);
    analogWrite(pinArduino, 0);
    break;
  default:
    xprintf(F("Pin %d unknown type %d\n"), pinArduino, pinType);
    break;
  }

  value.prevValue = value.currentValue;
}

bool pinHandler_t::updateValue()
{
  if (debounceCounter > 0) {
    debounceCounter--;
    return false;
  }

  switch (pinType) {
  case digitalInput:
    value.currentValue = digitalRead(pinArduino);
    if (value.prevValue != value.currentValue) {
      value.prevValue = value.currentValue;
      debounceCounter = debounceDelay;
      return true;
    }
    break;
  case digitalOutput:
  case socketOutput:
  case socketInput:
    value.currentValue = digitalRead(pinArduino);
    break;
  case pwmOutput:
    break;
  case analogInput:
    {
      uint16_t newValue = analogRead(pinArduino);
      // Apply IIR filter with coeff 0.25
      cli();
      value.currentValue -= value.currentValue >> denoiseFilterCoeff;
      value.currentValue += newValue;
      sei();
      int delta = abs((int)value.prevValue - (int)value.currentValue);
      if (delta > denoiseThreshold) {
        value.prevValue = value.currentValue;
        return true;
      }
    }
    break;
  default:
    break;
  }

  return false;
}

void pinHandler_t::setValue(uint8_t v)
{
  switch (pinType) {
  case digitalOutput:
  case socketOutput:
    digitalWrite(pinArduino, v);
    break;
  case pwmOutput:
    value.currentValue = v;
    analogWrite(pinArduino, v);
  case digitalInput:
  case socketInput:
  case analogInput:
    break;
  default:
    break;
  }
}

uint16_t pinHandler_t::getValue()
{
  if (pinType == analogInput) return value.currentValue >> denoiseFilterCoeff;
  else return value.currentValue;
}

connection_t pinHandler_t::getConnection()
{
  return connection.getConnection();
}

void pinHandler_t::serialOut(uint16_t bitNumber)
{
  if (bitNumber == 0) connection.serialBuffer = (Module.getModuleId() << 8) | pinId;
  digitalWrite(pinArduino, (connection.serialBuffer >> bitNumber) & 1);
}

bool pinHandler_t::serialIn(uint16_t bitNumber)
{
  uint16_t bit = digitalRead(pinArduino);
  if (bitNumber == 0) connection.serialBuffer = 0;
  connection.serialBuffer |= bit << bitNumber;

  if (bitNumber != 15) return false; // 16 bits not yet received

  if (connection.prevSerialBuffer != connection.serialBuffer) { // Received ID not confirmed, probably glitch during cable connection
    connection.prevSerialBuffer = connection.serialBuffer;
    return false; 
  }

  if (connection.isConnected && connection.serialBuffer == 0xFFFF) {  // Disconnection
    connection.isConnected = false;
    return true;
  }
  else if (!connection.isConnected && connection.serialBuffer != 0xFFFF) {  // New connection
    connection.confirmedConnection = connection.serialBuffer;
    connection.isConnected = true;
    return true;
  }

  return false;
}
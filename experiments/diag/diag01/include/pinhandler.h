#pragma once

#include <Arduino.h>
#include <ArduinoSTL.h>

#include <vector>
#include <map>
#include <iterator>

using namespace std;

typedef enum { undefined=-1, analogInput, digitalInput, digitalOutput, socketInput, socketOutput, pwmOutput } pinType_t;

typedef struct {
  uint8_t moduleId;
  uint8_t pinId;
  bool isConnected;
} connection_t;

class connectionManager_t {
public:
  uint16_t serialBuffer = 0xFFFF;       // Store the ID for this pin for synchronous serial transmission/reception
  uint16_t prevSerialBuffer = 0xFFFF;   // Used to confirm connection
  uint16_t confirmedConnection = 0xFFFF;
  bool isConnected = false;
  bool changed[2] = { false, false };   // we have to independant readers: I2C and console

  connection_t getConnection() { return { (uint8_t)(confirmedConnection >> 8), (uint8_t)(confirmedConnection & 0xFF), isConnected }; };
  void setId(uint8_t pinId) { serialBuffer = pinId; };
};

typedef struct {
  uint16_t prevValue = 0;       // Previous value, used for change detection
  uint16_t currentValue = 0;    // Current Value for this pin
} value_t;

struct pinHandler_t {
protected:
  uint8_t pinArduino;           // Physical pin ID
  pinType_t pinType;
  uint8_t pinId;

  connectionManager_t connection;
  value_t value;

  uint8_t debounceCounter = 0;

  static uint8_t debounceDelay;       // Delay before successive read of digital input for debouncing
  static uint8_t denoiseFilterCoeff;  // Anlog filter constant will 1<<analogFilterCoeff 
  static uint8_t denoiseThreshold;    // Threshold for change detection of analog inputs

public:
  pinHandler_t(pinType_t pinType, uint8_t pinArduino, uint8_t pinId);

  bool updateValue();   // Return true of value change
  uint16_t getValue();
  void setValue(uint8_t value);
  
  uint8_t getPinArduino() { return pinArduino; };
  #ifdef ATMEGA_4809
  String stringPinArduino() { return (pinType == analogInput) ?  ('A' + String(pinArduino)) : String(pinArduino); };
  #else
  String stringPinArduino() { return (pinArduino < A0) ? String(pinArduino) : 'A' + String(pinArduino-A0); };
  #endif

  connection_t getConnection();

  //void setPin(uint8_t value) { digitalWrite(pinArduino, value); };
  //uint8_t getPin() { return digitalRead(pinArduino); };

  void serialOut(uint16_t bitNumber);
  bool serialIn(uint16_t bitNumber);    // Return true if connection change
};

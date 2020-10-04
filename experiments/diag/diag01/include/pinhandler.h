#pragma once

#include <Arduino.h>
#include <ArduinoSTL.h>

#include <vector>
#include <map>
#include <iterator>

using namespace std;

typedef enum { undefined=-1, analogInput, digitalInput, digitalOutput, socketInput, socketOutput } pinType_t;

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
  bool changed = false;

  connection_t getConnection() { return { (uint8_t)(confirmedConnection >> 8), (uint8_t)(confirmedConnection & 0xFF), isConnected }; };
  void setId(uint8_t pinId) { serialBuffer = pinId; };
};

typedef struct {
  uint16_t prevValue = 0;       // Previous value, used for change detection
  uint16_t currentValue = 0;    // Current Value for this pin
  bool changed = false;
} value_t;

struct pinHandler_t {
protected:
  uint8_t pinArduino;           // Physical pin ID
  pinType_t pinType;
  uint8_t pinId;

  connectionManager_t connection;
  value_t value;

  uint8_t debounceCounter = 0;
  uint8_t debounceDelay = 5;  // Delay before successive read of digital input for debouncing

public:
  pinHandler_t(pinType_t pinType, uint8_t pinArduino, uint8_t pinId);

  void updateValue();
  uint16_t getValue(bool *hasChanged = NULL);
  void setBitValue(uint8_t value);

  uint8_t getPinArduino() { return pinArduino; };

  connection_t getConnection(bool* hasChanged = NULL);

  //void setPin(uint8_t value) { digitalWrite(pinArduino, value); };
  //uint8_t getPin() { return digitalRead(pinArduino); };

  void serialOut(uint16_t bitNumber);
  void serialIn(uint16_t bitNumber);
};

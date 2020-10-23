#include <ArduinoSTL.h>
#include <Wire.h>

#include "module.h"
#include "console.h"
#include "sysinfo.h"

void receiveEvent(int howMany);
void requestEvent();

struct {
  uint16_t onReceiveCount = 0;
  uint16_t onRequestCount = 0;
  uint16_t stepConnectionsCount = 0;
} I2C_stats;

void setup_I2C() {
  Wire.begin(Module.getModuleId());
  TWAR = (Module.getModuleId() << 1) | 1; // enable broadcasts to be received http://www.gammon.com.au/i2c
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void I2C_dump_stats() {
  xprintf(F("recv=%d, req=%d, step=%d\n"), 
    I2C_stats.onReceiveCount,
    I2C_stats.onRequestCount,
    I2C_stats.stepConnectionsCount);
}

void defaultConfig()
{
  Module.setModuleId(0x04);
  Module.definePins(analogInput, {A0, A1, A2, A3, A6, A7});
  Module.definePins(digitalInput,{2, 3});
  Module.definePins(digitalOutput,{13});
  Module.definePins(socketInput,{4, 5, 6, 7});
  Module.definePins(socketOutput,{8, 9, 10});
}

void setup() {
  sysinfo.wipeRam();
  Serial.begin(9600);
  xprintf(F("Starting\n"));

  // Loading config stored in EEPROM

  if (!Module.loadConfig()) defaultConfig();

  sysinfo.dumpStats();
  Module.dumpConfig();
  setup_I2C();
}

uint16_t counter = 0;
bool i2c_active = false;
bool trace_mode = true;

void loop() {
  // Poll socket connections every 100 ms
  // This is for standalone operation only
  // For production system, polling will be done via I2C broadcast messages
  if (!i2c_active && (counter % 10) == 0) Module.detectConnections();

  // Read all pins physical levels
  Module.updateAll(); 

  // Dump all changes (inputs level or socket connection);
  if (trace_mode) Module.dumpChanges();

  // Toggle a LED to check digital output feature is working
  if ((counter % 25) == 0) Module.blinkDigitalOutputs();

  // Check memory usage every 100ms
  if ((counter % 10) == 0) sysinfo.checkMemory();

  // I2C stats every second
  //if ((counter % 100) == 0) I2C_dump_stats();

  delay(10);
  counter += 1;

  pollCLI();
}

uint8_t message[8];
uint8_t I2C_tx_len = 0;
uint8_t I2C_maxSize = 32;


// The following default values can be altered by writing to I2C register 32

void receiveEvent(int howMany) 
{
  byte tickNum;
  I2C_stats.onReceiveCount++;
  if (!i2c_active) trace_mode = false; // On first I2C activation, disable trace changes on console
  i2c_active = true;

  for (uint8_t i=0; i<howMany ; i++) {
    byte data = Wire.read();
    if (i<sizeof(message)) message[i] = data;
  }

  switch (message[0]) {
    case 0:   // Tick message
      tickNum = message[1];
      if (tickNum > 32) Serial.println(F("I2C: bad tickNum"));
      Module.stepConnections(tickNum);
      I2C_stats.stepConnectionsCount++;
      break;
    case 1:   // changes reporting
      break;
    case 2:   // resend all states
      Serial.println(F("I2C: request full state"));
      Module.requestFullState();
      break;
    case 32:  // Configure I2C message size
      I2C_maxSize = message[1];
      xprintf(F("I2C: setting max size to %d\n"), I2C_maxSize);
      break;
    default:  // Space for other message types
      break;
  }
}

#define I2C_WRITE(b) { I2C_tx_len++; Wire.write(b); }

#define I2C_TAG_ANALOG_VALUE   0b00000000
#define I2C_TAG_DIGITAL_VALUE  0b01000000
#define I2C_TAG_CONNECTION     0b10000000
#define I2C_TAG_END            0b11111111

// For connections, reporting the format is a list of 3 bytes long records, ended by 0xFF
// record[0] = to pinId + I2C_TAG_CONNECTION
// record[1] = from moduleId + bit 7 = connection
// record[2] = from pinId
// moduleId should be <= 126
// pinId should be < 63

void reportConnections()
{
  
  while (I2C_tx_len < I2C_maxSize - 3) {
    connectionChangeEvent_t event;
    if (!Module.getNextConnectionChange(event)) break;  // No more connection change to send over I2C
    I2C_WRITE(event.pinId | I2C_TAG_CONNECTION);
    I2C_WRITE(event.from.moduleId + (event.from.isConnected ? 0x80 : 00));
    I2C_WRITE(event.from.pinId);
  }
}

// For analog values, reporting the format is a list of 3 bytes long records, ended by 0xFF
// record[0] = pinId | I2C_TAG_ANALOG_VALUE
// record[1] = new value high 8 bits
// record[2] = new value low 8 bits
// pinId should be < 63

void reportAnalogInputs()
{ 
  while (I2C_tx_len < I2C_maxSize - 3) {
    valueChangeEvent_t event;
    if (!Module.getNextAnalogInputChange(event)) break;  // No more connection change to send over I2C
    I2C_WRITE(event.pinId | I2C_TAG_ANALOG_VALUE);
    I2C_WRITE(highByte(event.newValue));
    I2C_WRITE(lowByte(event.newValue));
  }
}

// For digital values, reporting the format is a list of 2 bytes long records, ended by 0xFF
// record[0] = pinId | I2C_TAG_DIGITAL_VALUE
// record[1] = value
// pinId should be < 63

void reportDigitalInputs()
{ 
  while (I2C_tx_len < I2C_maxSize - 2) {
    valueChangeEvent_t event;
    if (!Module.getNextDigitalInputChange(event)) break;  // No more connection change to send over I2C
    I2C_WRITE(event.pinId | I2C_TAG_DIGITAL_VALUE);
    I2C_WRITE(lowByte(event.newValue));
  }
}

void requestEvent()
{
  I2C_stats.onRequestCount++;
  if (message[0] != 1) return;

  I2C_tx_len = 0;

  reportAnalogInputs();
  reportDigitalInputs();
  reportConnections();

  if (I2C_tx_len < I2C_maxSize) I2C_WRITE(I2C_TAG_END);
  if (I2C_tx_len < I2C_maxSize) I2C_WRITE(I2C_TAG_END);
}
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

void loop() {
  // Poll socket connections every 100 ms
  // This is for standalone operation only
  // For production system, polling will be done via I2C broadcast messages
  if (!i2c_active && (counter % 10) == 0) Module.detectConnections();

  // Read all pins physical levels
  Module.updateAll(); 

  // Dump all changes (inputs level or socket connection);
  Module.dumpChanges();

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

// The following default values can be altered by writing to I2C register 32

struct {
  uint8_t connectionReporting = 32;
  uint8_t analogReporting = 32;
  uint8_t digitalReporting = 32;
} I2C_maxSize;

void receiveEvent(int howMany) 
{
  byte tickNum;
  I2C_stats.onReceiveCount++;
  i2c_active = true;

  for (uint8_t i=0; i<howMany ; i++) {
    byte data = Wire.read();
    if (i<sizeof(message)) message[i] = data;
  }

  switch (message[0]) {
    case 0:   // Tick message
      tickNum = message[1];
      if (tickNum > 32) Serial.println("bad tickNum");
      Module.stepConnections(tickNum);
      I2C_stats.stepConnectionsCount++;
      break;
    case 1:   // Connections reporting
      break;
    case 2:   // Analog inputs reporting
      break;
    case 3:   // Digital inputs reporting
      break;
    case 32:  // Configure I2C message size
      I2C_maxSize.connectionReporting = message[1];
      I2C_maxSize.analogReporting = message[2];
      I2C_maxSize.digitalReporting = message[3];
      xprintf(F("Setting I2C max size to %d %d %d\n"), 
              I2C_maxSize.connectionReporting,
              I2C_maxSize.analogReporting,
              I2C_maxSize.digitalReporting);
      break;
    default:  // Space for other message types
      break;
  }
}

#define I2C_WRITE(b) { I2C_len++; Wire.write(b); }
#define I2C_CONNECTIONS_MAX_SIZE 32

// For connections, reporting the format is a list of 3 bytes long records, ended by 0xFF
// record[0] = from moduleId + bit 7 = connection
// record[1] = from pinId
// record[2] = to pinId (this module is the connection destination)
// moduleId should be <= 126

void reportConnections()
{
  uint8_t I2C_len = 0;
  
  while (I2C_len < I2C_maxSize.connectionReporting - 3) {  // The total I2C message size is limited to 32 bytes
    connectionChangeEvent_t event;
    if (!Module.getNextConnectionChange(event)) break;  // No more connection change to send over I2C
    I2C_WRITE(event.from.moduleId + (event.from.isConnected ? 0x80 : 00));
    I2C_WRITE(event.from.pinId);
    I2C_WRITE(event.pinId);
  }

  // End of list marker, if enough room
  if (I2C_len < 32) I2C_WRITE(0xFF);
  if (I2C_len < 32) I2C_WRITE(0xFF);
}

// For analog values, reporting the format is a list of 3 bytes long records, ended by 0xFF
// record[0] = pinId (or 0xFF for end of list)
// record[1] = new value high 8 bits
// record[2] = new value low 8 bits
// pinID should be <= 254

void reportAnalogInputs()
{
  uint8_t I2C_len = 0;
  
  while (I2C_len < I2C_maxSize.analogReporting - 3) {  // The total I2C message size is limited to 32 bytes
    valueChangeEvent_t event;
    if (!Module.getNextAnalogInputChange(event)) break;  // No more connection change to send over I2C
    I2C_WRITE(event.pinId);
    I2C_WRITE(highByte(event.newValue));
    I2C_WRITE(lowByte(event.newValue));
  }

  // End of list marker, if enough room
  if (I2C_len < 32) I2C_WRITE(0xFF);
  if (I2C_len < 32) I2C_WRITE(0xFF);
}

// For digital values, reporting the format is a list of 2 bytes long records, ended by 0xFF
// record[0] = pinId (or 0xFF for end of list)
// record[1] = value
// pinID should be <= 254

void reportDigitalInputs()
{
  uint8_t I2C_len = 0;
  
  while (I2C_len < I2C_maxSize.digitalReporting - 2) {  // The total I2C message size is limited to 32 bytes
    valueChangeEvent_t event;
    if (!Module.getNextDigitalInputChange(event)) break;  // No more connection change to send over I2C
    I2C_WRITE(event.pinId);
    I2C_WRITE(lowByte(event.newValue));
  }

  // End of list marker, if enough room
  if (I2C_len < 32) I2C_WRITE(0xFF);
  if (I2C_len < 32) I2C_WRITE(0xFF);
}

void requestEvent()
{
  I2C_stats.onRequestCount++;


  switch (message[0]) {
    case 1: reportConnections(); break;
    case 2: reportAnalogInputs(); break;
    case 3: reportDigitalInputs(); break;
    default: break;
  }

}
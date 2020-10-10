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
  if (!i2c_active) Module.dumpChanges();

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

void receiveEvent(int howMany) {
  byte message[2];
  byte tickNum;

  I2C_stats.onReceiveCount++;

  i2c_active = true;

  for (uint8_t i=0; i<howMany ; i++){
    byte data = Wire.read();
    if (i<sizeof(message)) message[i] = data;
  }
  
  if (howMany != 2) return; // Bad message

  switch (message[0]) {
    case 0:   // Tick message
      tickNum = message[1];
      if (tickNum > 32) Serial.println("bad tickNum");
      Module.stepConnections(tickNum);
      I2C_stats.stepConnectionsCount++;
      break;
    default:  // Space for other message types
      break;
  }
}

void requestEvent() {
  byte numNewConnected = 0;
  byte numNewDisconnected = 0;

  I2C_stats.onRequestCount++;

  connectionChangeList_t list = Module.getConnectionChangeList();

  // Count number of connections and disconnections

  for (uint8_t i = 0; i < list.size(); i++) {
    if (list[i].from.isConnected) numNewConnected++;
    else numNewDisconnected++;
  }

  Wire.write(numNewConnected);
  Wire.write(numNewDisconnected);

  // Send list of connections, and then list of disconnections
  // In the following loop, nc==0 means connections, nc==1 means disconnections

  for (uint8_t nc=0; nc<=1; nc++) {
    for (uint8_t i = 0; i < list.size(); i++) {
      if (list[i].from.isConnected == nc) continue;  // Process only the right event type
      Wire.write(list[i].from.moduleId);
      Wire.write(list[i].from.pinId);
      Wire.write(Module.getModuleId());
      Wire.write(list[i].pinId);
    }
  }
}
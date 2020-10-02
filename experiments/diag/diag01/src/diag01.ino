#include <ArduinoSTL.h>
#include <Wire.h>
//#include <EEPROM.h>

#include "module.h"
#include "console.h"

void receiveEvent(int howMany);
void requestEvent();

uint8_t moduleId = 0x55;
Module_t *Module = NULL;

void setup_I2C() {
  Wire.begin(moduleId);
  TWAR = (moduleId << 1) | 1; // enable broadcasts to be received http://www.gammon.com.au/i2c
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void setup() {
  Serial.begin(9600);
  xprintf("Starting");

  // Create Module and define mapping of Arduino physical pins

  Module = new Module_t(moduleId);

  Module->definePins(analogInput, {A0, A1, A2, A3, A6, A7});
  Module->definePins(digitalInput,{2, 3});
  Module->definePins(digitalOutput,{13});
  Module->definePins(socketInput,{4, 5, 6, 7});
  Module->definePins(socketOutput,{8, 9, 10});
}

uint16_t counter = 0;

void loop() {
  // Poll socket connections every 100 ms
  // This is for standalone operation only
  // For production system, polling will be done via I2C broadcast messages
  if ((counter % 10) == 0) Module->detectConnections();

  // Read all pins physical levels
  Module->updateAll(); 
  
  // Dump all values every 30 seconds
  if ((counter % 3000) == 0) Module->dumpValues();

  // Dump all changes (inputs level or socket connection);
  Module->dumpChanges();

  // Toggle a LED to check digital output feature is working
  if ((counter % 25) == 0) Module->setValue(digitalOutput, 0, 1 - Module->getValue(digitalOutput, 0));
  delay(10);
  counter += 1;
}

void receiveEvent(int howMany) {
  byte message[2];
  byte tickNum;

  for (uint8_t i=0; i<howMany ; i++){
    if (i<sizeof(message)) message[i] = Wire.read();
  }
  
  if (howMany != 2) return; // Bad message

  switch (message[0]) {
    case 0:   // Tick message
      tickNum = message[1];
      Module->stepConnections(tickNum);
      break;
    default:  // Space for other message types
      break;
  }
}

void requestEvent() {
  byte numNewConnected = 0;
  byte numNewDisconnected = 0;

  connectionChangeList_t list = Module->getConnectionChangeList();
  if (list.size() == 0) return;

  // Count number of connections and disconnections

  for (uint8_t i = 0; i < list.size(); i++) {
    if (list[i].from.isConnected) numNewConnected++;
    else numNewDisconnected++;
   }

  Wire.write(numNewConnected);
  Wire.write(numNewDisconnected);

  // Send list of connections, and then list of disconnections
  // In the following loop, c==1 means connections, c==0 means disconnections
  
  for (uint8_t c=1; c>=0; c--) {
    for (uint8_t i = 0; i < list.size(); i++) {
      if (list[i].from.isConnected != c) continue;  // Process only the right event type
      Wire.write(list[i].from.moduleId);
      Wire.write(list[i].from.pinId);
      Wire.write(moduleId);
      Wire.write(list[i].pinId);
    }
  }
}
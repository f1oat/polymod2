
#include <ArduinoSTL.h>
#include "module.h"

Module_t *Module = new Module_t(0x55);

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");

  // Define mapping of Arduino physical pins

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

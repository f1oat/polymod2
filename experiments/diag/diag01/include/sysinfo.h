#pragma once

class sysinfoClass {
public:
	static void wipeRam(); 		// Wipe RAM between heap and stack with 0x55 pattern
	static int freeRam();		// get free RAM (heap size)
	static int unusedRam();		// get number of byte not used (by checking a memory pattern wirtten at boot)
	static int stackUsage();
	static void dumpStats();
	static void checkMemory();	// Check memory usage and warn if 
};

extern sysinfoClass sysinfo;
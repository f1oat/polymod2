#include <Arduino.h>
#include "sysinfo.h"

#include "console.h"

int sysinfoClass::freeRam()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// Wipe RAM between heap and stack with 0x55 pattern

void sysinfoClass::wipeRam()
{
	extern int __heap_start, *__brkval;
	int v;
	for (byte *ptr = (byte *)(__brkval == 0 ? &__heap_start : __brkval); ptr < (byte *)&v; ptr++) {
		*ptr = 0x55;
	}
}

// Measure stack usage

int sysinfoClass::stackUsage()
{
	extern int __stack;
	extern int __heap_start, *__brkval;

	short *heap_top = (short *)(__brkval == 0 ? &__heap_start : __brkval);
	short *ptr = (short *)&__stack;

	int count = 0;
	while (*ptr != 0x5555 && ptr >= heap_top) {
		ptr--;
		count += 2;
	}

	return count;
}

// Measure unused Ram

int sysinfoClass::unusedRam()
{
	int count = 0;
	for (short *ptr = 0; (int)ptr <= RAMEND; ptr++) {
		if (*ptr == 0x5555) count += 2;
	}
	return count;
}

void sysinfoClass::dumpStats()
{	
	xprintf(F("millis:      %lu\n"), millis());
	xprintf(F("Free Ram:    %d\n"), freeRam());
	xprintf(F("Unused Ram:  %d\n"), unusedRam());
	xprintf(F("Stack usage: %d\n"), stackUsage());
}

void sysinfoClass::checkMemory()
{
	static bool warning = false;
	int threshold = 32;	// We need at least this number of bytes of margin
	
	if (warning) return;
	int u = unusedRam();
	if (threshold > u) return;
	warning = true;
	xprintf(F("*** warning: memory low (%d unused bytes < %d)\n"), u, threshold);
}

sysinfoClass sysinfo;
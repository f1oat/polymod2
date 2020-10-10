#pragma once

#include <EEPROM.h>
#include <Arduino.h>

class NVMEMClass {
public:
	bool CheckCRC(void);
	void UpdateCRC(void);

	void rewind() { p = 0; };

    template< typename T > T &get( T &t ){
		EEPROM.get(p, t);
		//xprintf(F("get(%d) => %d\n"), p, t);
		p += sizeof(T);
		return t;
    }
    
    template< typename T > const T &put( const T &t ){
		EEPROM.put(p, t);
		//xprintf(F("put(%d, %d)\n"), p, t);
		p += sizeof(T);
		return t;
    }

	bool eof()  { return p >= EEPROM.length(); }

private:
	uint16_t p = 0;
	uint16_t version;
	const uint16_t _version = 0x07FC;
	uint16_t eeprom_crc(uint16_t minus = 0);

};

extern NVMEMClass NVMEM;


#include "nvmem.h"
#include "console.h"

NVMEMClass NVMEM;;

// Code coming from http://www.ccontrolsys.com/w/How_to_Compute_the_Modbus_RTU_Message_CRC

uint16_t NVMEMClass::eeprom_crc(uint16_t minus)
{
	uint16_t crc = ~0L;

	for (uint16_t index = 0; index < EEPROM.length() - minus; ++index) {
		crc ^= EEPROM[index];          // XOR byte into least sig. byte of crc

		for (int i = 8; i != 0; i--) {    // Loop over each bit
			if ((crc & 0x0001) != 0) {      // If the LSB is set
				crc >>= 1;                    // Shift right and XOR 0xA001
				crc ^= 0xA001;
			}
			else                            // Else LSB is not set
				crc >>= 1;                    // Just shift right
		}                    // Just shift right
	}
	return crc;
}

bool NVMEMClass::CheckCRC(void)
{
	// The following CRC check is a classical method, used for HDLC for example
	// When CRC calculation is included the CRC in the last word, the result is 0
	if (eeprom_crc(0) != 0) {
		xprintf(F("Bad EEPROM CRC\n"));
		return false;
	}

	EEPROM.get(EEPROM.length() - 4, version);
	if (version != _version) {
		xprintf(F("Bad version %04X (waiting %04X)\n"), _version);
		return false;
	}

	return true;
}

void NVMEMClass::UpdateCRC(void)
{
	// Put version at the end, just before the CRC
	EEPROM.put(EEPROM.length() - 4, _version);
	
	// Compute the CRC and write at the end
	uint16_t crc = eeprom_crc(2);
	EEPROM.put(EEPROM.length() - 2, crc);
}

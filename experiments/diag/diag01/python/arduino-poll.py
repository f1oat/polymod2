import sys
import time

from pythonosc import udp_client
import smbus

bus = smbus.SMBus(1)    # 0 = /dev/i2c-0 (port I2C0), 1 = /dev/i2c-1 (port I2C1)
modules = [ 4 ];

changesMaxSize = 4*3    # Up to 4 changes

I2C_TAG_ANALOG_VALUE   = 0b00000000
I2C_TAG_DIGITAL_VALUE  = 0b01000000
I2C_TAG_CONNECTION     = 0b10000000
I2C_TAG_END            = 0b11111111
I2C_TAG_MASK           = 0b11000000

def testConnections():
	for tickNum in range(0, 32):
		bus.write_byte_data(0, 0, tickNum)	# Broadcast, offset=0 means tick message
	return

def configureMessageSize():
	bus.write_i2c_block_data(0, 32, [ changesMaxSize ])

def getChanges():
	for addr in modules:
		pdu = bus.read_i2c_block_data(addr, 1, changesMaxSize) # Offset=1 means get list of changes
		ptr = 0
		while (pdu[ptr] != I2C_TAG_END and ptr < changesMaxSize):
			tag = pdu[ptr] & I2C_TAG_MASK
			if (tag == I2C_TAG_ANALOG_VALUE):
				pinId = pdu[ptr] & ~I2C_TAG_MASK
				value = (pdu[ptr+1] << 8) + pdu[ptr+2]
				ptr += 3
				print("Analog m%dp%d = %d" % (addr, pinId, value))
			elif (tag == I2C_TAG_DIGITAL_VALUE):
				pinId = pdu[ptr] & ~I2C_TAG_MASK
				value = pdu[ptr+1]
				ptr += 2
				print("Digital m%dp%d = %d" % (addr, pinId, value))
			elif (tag == I2C_TAG_CONNECTION):
				toModule = addr
				toPort = pdu[ptr] & ~I2C_TAG_MASK
				fromModule = pdu[ptr+1] & 0x7F
				connected = pdu[ptr+1] & 0x80
				fromPort = pdu[ptr+2]
				ptr += 3
				if (connected): print("connected: m%dp%d -> m%dp%d" % (fromModule, fromPort, toModule, toPort))
				else: print("disconnected: m%dp%d -> m%dp%d" % (fromModule, fromPort, toModule, toPort))



if __name__ == "__main__":
	client = udp_client.SimpleUDPClient('127.0.0.1', 9001)
	#url = sys.argv[1]
	#args = sys.argv[2:]
	#args = list(map(int, args))
	#client.send_message(url, args)

	while True:
		try:
			configureMessageSize()
			while True:
				testConnections()
				getChanges()
		except OSError:
			print("I2C error")
			time.sleep(1);

		time.sleep(1e-3)

	
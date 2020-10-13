import sys
import time

from pythonosc import udp_client
import smbus

bus = smbus.SMBus(1)    # 0 = /dev/i2c-0 (port I2C0), 1 = /dev/i2c-1 (port I2C1)
modules = [ 4 ];

connectionReportingMaxSize = 4*3;
analogReportingMaxSize = 2*3;
digitalReportingMaxSize = 2*2;

def testConnections():
	for tickNum in range(0, 32):
		bus.write_byte_data(0, 0, tickNum)	# Broadcast, offset=0 means tick message
	return

def configureMessageSize():
	bus.write_i2c_block_data(0, 32, [connectionReportingMaxSize, analogReportingMaxSize, digitalReportingMaxSize])

def getConnectionChanges():
	for addr in modules:
		pdu = bus.read_i2c_block_data(addr, 1, connectionReportingMaxSize) # Offset=1 means connections polling
		ptr = 0
		while (pdu[ptr] != 0xFF and ptr <= 30):
			fromModule = pdu[ptr] & 0x7F
			connected = pdu[ptr] & 0x80
			fromPort = pdu[ptr+1]
			toModule = addr
			toPort = pdu[ptr+2]
			ptr += 3
			if (connected): print("connected: m%dp%d -> m%dp%d" % (fromModule, fromPort, toModule, toPort))
			else: print("disconnected: m%dp%d -> m%dp%d" % (fromModule, fromPort, toModule, toPort))

def getAnalogInputChanges():
	for addr in modules:
		pdu = bus.read_i2c_block_data(addr, 2, analogReportingMaxSize) # Offset=2 means analog inputs polling
		ptr = 0
		while (pdu[ptr] != 0xFF and ptr <= 30):
			pinId = pdu[ptr]
			value = (pdu[ptr+1] << 8) + pdu[ptr+2]
			ptr += 3
			print("Analog m%dp%d = %d" % (addr, pinId, value))

def getDigitalInputChanges():
	for addr in modules:
		pdu = bus.read_i2c_block_data(addr, 3, digitalReportingMaxSize) # Offset=3 means analog inputs polling
		ptr = 0
		while (pdu[ptr] != 0xFF and ptr <= 30):
			pinId = pdu[ptr]
			value = pdu[ptr+1]
			ptr += 2
			print("Digital m%dp%d = %d" % (addr, pinId, value))

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
				getConnectionChanges()
				getAnalogInputChanges()
				getDigitalInputChanges()
		except OSError:
			print("I2C error")
			time.sleep(1);

		time.sleep(1e-3)

	
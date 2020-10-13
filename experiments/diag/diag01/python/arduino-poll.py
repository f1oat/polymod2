import sys
import time

from pythonosc import udp_client
import smbus

bus = smbus.SMBus(1)    # 0 = /dev/i2c-0 (port I2C0), 1 = /dev/i2c-1 (port I2C1)
modules = [ 4 ];

def testConnections():
	for tickNum in range(0, 32):
		bus.write_byte_data(0, 0, tickNum)	# Broadcast, offset=0 means tick message
	return

def getChanges():
	for addr in modules:
		pdu = bus.read_i2c_block_data(addr, 1, 8) # Offset=1 means connections polling
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

if __name__ == "__main__":
	client = udp_client.SimpleUDPClient('127.0.0.1', 9001)
	#url = sys.argv[1]
	#args = sys.argv[2:]
	#args = list(map(int, args))
	#client.send_message(url, args)

	phase = ""

	while True:
		try:
			phase = "connnections testing"
			testConnections()
			phase = "changes collection"
			getChanges()
		except OSError:
			print("I2C error during " + phase)
			time.sleep(1);

		time.sleep(1e-3)

	
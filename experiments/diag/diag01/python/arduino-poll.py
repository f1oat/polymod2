# OSC messages format
# 
# *** Emitted by the control panel:
#
# /matrix/reset
# /matrix/connect <dst_module> <dst_port> <src_module> <src_port>
# /matrix/disconnect <dst_module> <dst_port> <src_module> <src_port>
# /module/analog <module> <channel> <value>
# /module/digital <module> <channel> <0|1>
# 
# *** Received by the control panel:
#
# /reset
# /module/digital <module> <channel> <0|1>

import sys
import time
from typing import List, Any

from pythonosc import udp_client
from pythonosc import dispatcher

from pythonosc.osc_server import AsyncIOOSCUDPServer
from pythonosc.osc_server import BlockingOSCUDPServer
from pythonosc.dispatcher import Dispatcher
import asyncio

import smbus

bus = smbus.SMBus(1)    # 0 = /dev/i2c-0 (port I2C0), 1 = /dev/i2c-1 (port I2C1)
modules = [ 4 ];

changesMaxSize = 4*3    # Up to 4 changes

# Received I2C messages
I2C_TAG_ANALOG_VALUE   = 0b00000000
I2C_TAG_DIGITAL_VALUE  = 0b01000000
I2C_TAG_CONNECTION     = 0b10000000
I2C_TAG_END            = 0b11111111
I2C_TAG_MASK           = 0b11000000

# Transmitted I2C messages

I2C_TICK               = 0
I2C_GET_CHANGES        = 1
I2C_REQUEST_FULLSTATE  = 2
I2C_WRITE_DIGITAL	   = 3
I2C_SET_CONFIG         = 32

def testConnections():
	for tickNum in range(0, 32):
		bus.write_byte_data(0, I2C_TICK, tickNum)
	return

def configureMessageSize():
	bus.write_i2c_block_data(0, I2C_SET_CONFIG, [ changesMaxSize ])

def requestFullState():
	bus.write_byte_data(0, I2C_REQUEST_FULLSTATE, 0)

def getChanges():
	for addr in modules:
		pdu = bus.read_i2c_block_data(addr, I2C_GET_CHANGES, changesMaxSize)
		ptr = 0
		while (pdu[ptr] != I2C_TAG_END and ptr < changesMaxSize):
			oscMessage = []
			tag = pdu[ptr] & I2C_TAG_MASK
			if (tag == I2C_TAG_ANALOG_VALUE):
				pinId = pdu[ptr] & ~I2C_TAG_MASK
				value = (pdu[ptr+1] << 8) + pdu[ptr+2]
				ptr += 3
				#print("Analog m%dp%d = %d" % (addr, pinId, value))
				oscMessage = [ "/module/analog", [addr, pinId, value/1024.0] ]
			elif (tag == I2C_TAG_DIGITAL_VALUE):
				pinId = pdu[ptr] & ~I2C_TAG_MASK
				value = pdu[ptr+1]
				ptr += 2
				#print("Digital m%dp%d = %d" % (addr, pinId, value))
				oscMessage = [ "/module/digital", [ addr, pinId, value ] ]
			elif (tag == I2C_TAG_CONNECTION):
				toModule = addr
				toPort = pdu[ptr] & ~I2C_TAG_MASK
				fromModule = pdu[ptr+1] & 0x7F
				connected = pdu[ptr+1] & 0x80
				fromPort = pdu[ptr+2]
				ptr += 3
				#if (connected): print("connected: m%dp%d -> m%dp%d" % (fromModule, fromPort, toModule, toPort))
				#else: print("disconnected: m%dp%d -> m%dp%d" % (fromModule, fromPort, toModule, toPort))
				if (connected): oscMessage = [ "/matrix/connect", [toModule, toPort, fromModule, fromPort] ]
				else: 			oscMessage = [ "/matrix/disconnect" , [toModule, toPort, fromModule, fromPort] ]
			if (oscMessage != []):
				 print(oscMessage)
				 client.send_message(oscMessage[0], oscMessage[1])

def digitalOutputHandler(address: str, *args: List[Any]) -> None:
	print(address, args)
	bus.write_i2c_block_data(args[0], I2C_WRITE_DIGITAL, [ args[1], args[2] ])

def resetHandler(address: str, *args: List[Any]) -> None:
	print("reset")
	client.send_message("/matrix/reset", [])
	requestFullState()

def trashHandler(address: str, *args: List[Any]) -> None:
	print(address, args)

if __name__ == "__main__":
	server_ip = '0.0.0.0'
	client_ip = '127.0.0.1'

	if (len(sys.argv) > 1):
		client_ip = sys.argv[1]

	# Init the client side of OSC

	client = udp_client.SimpleUDPClient(client_ip, 9001)
	client.send_message("/matrix/reset", [])
	print("Sending OSC message to %s" % (client_ip))
	
	# Init the server side of OSC

	dispatcher = dispatcher.Dispatcher()
	dispatcher.map("/module/digital", digitalOutputHandler)
	dispatcher.map("/reset", resetHandler)
	dispatcher.set_default_handler(trashHandler)

	#server = BlockingOSCUDPServer((server_ip, 9001), dispatcher)
	#server.serve_forever()  # Blocks forever

	async def loop():
		while True:
			try:
				configureMessageSize()
				requestFullState()
				while True:
					testConnections()
					getChanges()
					await asyncio.sleep(0)
			except OSError:
				print("I2C error")
				await asyncio.sleep(1)

	async def init_main():
		server = AsyncIOOSCUDPServer((server_ip, 9001), dispatcher, asyncio.get_event_loop())
		transport, protocol = await server.create_serve_endpoint() 
		print("Serving on %s" % (server_ip))
		await loop() 
		transport.close()

	asyncio.run(init_main())


	

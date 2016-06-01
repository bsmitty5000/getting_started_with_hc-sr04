# -*- coding: utf-8 -*-
"""
Testing out HC-SR04 on dsPIC33f
"""
import serial
import struct
import signal
import sys
        
ser = serial.Serial(port = '/dev/ttyUSB1', 
                    baudrate = 115200,
                    bytesize = 8,
                    parity = serial.PARITY_NONE,
                    stopbits = 1,
                    timeout = 60)
                    
def signal_handler(signal, frame):
    print "exiting gracefully as a motherfucker"
    ser.close()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

raw_input("Push any key then enter to start: ")

ser.write('x')

print "Press ctrl-c to stop once it's running"

while 1:
    
    rcvd = ser.read(4)
    rcvd = struct.unpack('f', rcvd)
    print(str(rcvd[0]) + " cm")


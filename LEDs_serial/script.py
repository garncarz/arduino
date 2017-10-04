#!/usr/bin/env python3

# TODO use threads, so serial input can be read/printed on the fly

import getch
import serial

ser = serial.Serial('/dev/ttyS2', 9600)  # Cygwin
print(ser.read_all().decode().strip())

while True:
    try:
        key = getch.getch()
        ser.write([int(key)])
        print(ser.read_all().decode().strip())
    except Exception as e:
        print(e)

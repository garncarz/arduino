#!/usr/bin/env python3

import keyboard
import serial

LEDS = 6

ser = serial.Serial('COM3', 9600)  # Windows


def check_output():
    output = ''
    while True:
        output += ser.read_all().decode()
        if '\n' in output:
            line, output = output.split('\n', 1)

            # FIXME not sure where it goes, program gets here,
            # but nothing is printed
            print(line)


def send_key(e):
    if e.event_type == 'down':
        ser.write([e.scan_code % LEDS])


def main():
    keyboard.hook(send_key)
    check_output()
    keyboard.wait()  # probably not needed


if __name__ == '__main__':
    main()

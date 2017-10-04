#!/usr/bin/env python3

import re

import keyboard
import serial


ser = serial.Serial('COM3', 9600)  # Windows


with open('LEDs_serial.ino') as f:
    INO_LINES = f.read()


LEDS = int(re.search(r'const int LEDS = (\d+);', INO_LINES).group(1))


def get_ino_value(name, default):
    if len(name) > 1:
        m = re.search(r'case (\d+): .* //.*%s' % name, INO_LINES)
        if m:
            return int(m.group(1))
    return default


def check_output():
    output = ''
    while True:
        output += ser.read_all().decode()
        if '\n' in output:
            line, output = output.split('\n', 1)

            # Python (Win) / tmux (Cygwin): gets printed with quite a delay
            print(line)


def send_key(e):
    if e.event_type == 'down':
        value = get_ino_value(e.name, e.scan_code % LEDS)
        ser.write([value])


def main():
    keyboard.hook(send_key)
    check_output()
    keyboard.wait()  # probably not needed


if __name__ == '__main__':
    main()

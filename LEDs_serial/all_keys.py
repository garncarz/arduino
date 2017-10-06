#!/usr/bin/env python3

from functools import lru_cache
import re
import time

import keyboard
import mouse
import serial


PORT = 'COM3'  # Windows

last_write_time = time.time()

MOUSE_MIN_DIFF = 2
last_mouse_x, last_mouse_y = mouse.get_position()


with open('LEDs_serial.ino') as f:
    INO_LINES = f.read()


BAUDRATE = int(re.search(r'Serial.begin\((\d+)\);', INO_LINES).group(1))
LEDS = int(re.search(r'const int LEDS = (\d+);', INO_LINES).group(1))
DELAY = int(re.search(r'const int DELAY = (\d+);', INO_LINES).group(1)) / 1000.0  # seconds


ser = serial.Serial(PORT, BAUDRATE)


@lru_cache()
def get_ino_value(name):
    if len(name) > 1:
        m = re.search(r'case (\d+): .* //.*%s' % name, INO_LINES)
        if m:
            return int(m.group(1))


def check_output():
    output = ''
    while True:
        output += ser.read_all().decode()
        if '\n' in output:
            line, output = output.split('\n', 1)

            # Python (Win):
            #   - tmux (Cygwin): gets printed with quite a delay
            #   - PyCharm: immediate
            print(line)


def write(value, delay=DELAY):
    if not value:
        return False

    global last_write_time
    if value < 20 or time.time() > last_write_time + delay:
        try:
            ser.write([value])
        except Exception as e:
            print(e)
        last_write_time = time.time()
        return True

    return False


def send_key(e):
    if e.event_type == 'down':
        value = get_ino_value(e.name) or e.scan_code % LEDS
        write(value)


def send_mouse(e):
    global last_mouse_x, last_mouse_y
    value = None

    if isinstance(e, mouse.ButtonEvent):
        if e.event_type == mouse.DOUBLE:
            # double click isn't recognized? this doesn't seem to work
            value = get_ino_value('enter')
        elif e.event_type == mouse.DOWN:
            value = (
                1 if e.button == mouse.LEFT
                else 2 if e.button == mouse.RIGHT
                else 3
            )

    elif isinstance(e, mouse.WheelEvent):
        value = get_ino_value('page up' if e.delta > 0 else 'page down')

    elif isinstance(e, mouse.MoveEvent):
        x, y = mouse.get_position()
        if x < last_mouse_x - MOUSE_MIN_DIFF or y < last_mouse_y - MOUSE_MIN_DIFF:
            value = get_ino_value('up')
        elif x > last_mouse_x + MOUSE_MIN_DIFF or y > last_mouse_y + MOUSE_MIN_DIFF:
            value = get_ino_value('down')
        last_mouse_x, last_mouse_y = x, y

    write(value)


def main():
    keyboard.hook(send_key)
    mouse.hook(send_mouse)
    check_output()
    keyboard.wait()  # probably not needed


if __name__ == '__main__':
    main()

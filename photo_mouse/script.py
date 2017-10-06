#!/usr/bin/env python3

import logging
import re

import mouse
import serial


logger = logging.getLogger(__name__)

PORT = 'COM3'  # Windows

with open('photo_mouse.ino') as f:
    INO_LINES = f.read()

BAUDRATE = int(re.search(r'Serial.begin\((\d+)\);', INO_LINES).group(1))

ser = serial.Serial(PORT, BAUDRATE)


def main():
    while True:
        logger.debug('Waiting for input...')
        val = int(ser.read()[0])
        logger.debug(f'Got {val}')

        if val == 31:
            logger.info('Left click')
            mouse.click(mouse.LEFT)
        elif val == 41:
            logger.info('Middle click')
            mouse.click(mouse.MIDDLE)
        elif val == 51:
            logger.info('Right click')
            mouse.click(mouse.RIGHT)

        elif val == 1:
            logger.info('Move left')
            mouse.move(x=-1, y=0, absolute=False)
        elif val == 2:
            logger.info('Move right')
            mouse.move(x=1, y=0, absolute=False)
        elif val == 21:
            logger.info('Move up')
            mouse.move(y=-1, x=0, absolute=False)
        elif val == 22:
            logger.info('Move down')
            mouse.move(y=1, x=0, absolute=False)

        elif val == 11:
            logger.info('Wheel down')
            mouse.wheel(-1)
        elif val == 12:
            logger.info('Wheel up')
            mouse.wheel(1)


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)
    main()

#!/usr/bin/env python3

import argparse
from functools import lru_cache
import logging
import multiprocessing
import re
from time import sleep

import mido
import serial


logger = logging.getLogger(__name__)

arg_parser = argparse.ArgumentParser(
    description='Arduino MIDI player',
)
arg_parser.add_argument('file')

PORT = '/dev/ttyS2'  # Cygwin

with open('midi.ino') as f:
    INO_LINES = f.read()

BAUDRATE = int(re.search(r'Serial.begin\((\d+)\);', INO_LINES).group(1))

ser = serial.Serial(PORT, BAUDRATE)


@lru_cache()
def get_ino_value(name):
    if len(name) > 1:
        m = re.search(r'== (\d+).* // %s' % name, INO_LINES)
        if m:
            return int(m.group(1))


def check_output():
    output = ''
    while True:
        output += ser.read_all().decode()
        if '\n' in output:
            line, output = output.split('\n', 1)
            print(line)


def write(*values):
    try:
        ser.write([255] + list(values))
    except Exception as e:
        print(e)


def stop(tone=0):
    write(get_ino_value('stop'), tone)


def play_midi(filename):
    mid = mido.MidiFile(filename)

    for msg in mid.play():
        if msg.channel == 9:  # drums
            continue

        if msg.type == 'note_on':
            logger.debug(f'play {msg.note}')
            write(get_ino_value('play'), msg.note)
        elif msg.type == 'note_off':
            logger.debug(f'stop {msg.note}')
            stop(msg.note)

    stop()


def main():
    args = arg_parser.parse_args()

    watcher = multiprocessing.Process(target=check_output)
    sender = multiprocessing.Process(target=play_midi, args=(args.file,))

    try:
        watcher.start()
        sender.start()

        sender.join()
        sleep(3)
        watcher.terminate()

    except KeyboardInterrupt:
        stop()

        sender.terminate()
        watcher.terminate()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    main()

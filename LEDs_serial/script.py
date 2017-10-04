#!/usr/bin/env python3

import multiprocessing

import getch
import serial


ser = serial.Serial('/dev/ttyS2', 9600)  # Cygwin


def check_output():
    output = ''
    while True:
        output += ser.read_all().decode()
        if '\n' in output:
            line, output = output.split('\n', 1)
            print(line)


def send_keys():
    while True:
        try:
            key = getch.getch()
            ser.write([int(key)])
        except Exception as e:
            print(e)


def main():
    watcher = multiprocessing.Process(target=check_output)
    sender = multiprocessing.Process(target=send_keys)

    try:
        watcher.start()
        sender.start()

        watcher.join()
        sender.join()

    except KeyboardInterrupt:
        sender.terminate()
        watcher.terminate()


if __name__ == '__main__':
    main()

#!/usr/bin/env python3

import collections
import time

import numpy as np
import pyaudio

from all_keys import write


SAMPLE_PYAUDIO = pyaudio.paInt16
SAMPLE_NUMPY = np.int16
CHANNELS = 2
RATE = 44100

HISTORY = 100
TIMEFRAME = 0.1


max_samples = collections.deque(maxlen=int(HISTORY / TIMEFRAME))

p = pyaudio.PyAudio()


def callback(in_data, frame_count, time_info, status):
    samples = np.fromstring(in_data, dtype=SAMPLE_NUMPY)
    max_sample_now = np.max(np.abs(samples))
    max_samples.append(max_sample_now)
    max_sample_overall = np.max(max_samples) or 1
    ratio = max_sample_now / max_sample_overall
    write(10 + int(ratio * 10))
    return (None, pyaudio.paContinue)


stream = p.open(
    format=SAMPLE_PYAUDIO,
    channels=CHANNELS,
    rate=RATE,
    input=True,
    stream_callback=callback,
)


def main():
    stream.start_stream()

    while stream.is_active():
        time.sleep(TIMEFRAME)

    stream.stop_stream()
    stream.close()

    p.terminate()


if __name__ == '__main__':
    main()

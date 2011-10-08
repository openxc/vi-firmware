#!/usr/bin/env python

import time
import sys
import struct
import serial
import datetime

MAX_BYTES = 10 * 1000 * 10 * 5


class SerialDevice(object):
    def __init__(self, device="/dev/ttyUSB2", baud=2000000):
        self.device = serial.Serial(device, baud, timeout=10)
        self.device.flushInput()
        self.message_size = -1

    def initialize_message_size(self, message_size):
        self.message_size = message_size
        self.device.write(bytearray([self.message_size / 1000]))
        self.device.flushOutput()
        print "Message size switched to %d" % self.message_size
        self.bytes_received = 0

    def read(self):
        data = self.device.read(self.message_size)

        self.bytes_received += self.message_size
        if self.bytes_received % (1000 * 10) == 0:
            print "Received %d kilobytes so far..." % (
                    self.bytes_received / 1000),
            sys.stdout.flush()
            print "\r",
        return data

    def total_time(self, elapsed_time):
        return "Reading %s KB in %s byte chunks took %s" % (
                self.bytes_received / 1000, self.message_size, elapsed_time)

    def throughput(self, elapsed_time):
        return (
            "The effective throughput for %d byte messages is %d KB/s" % (
                self.message_size, self.bytes_received / 1000
                    / max(1, elapsed_time.seconds + elapsed_time.microseconds /
                        1000000.0)))

def run_benchmark(serial_device, message_size, total_bytes=MAX_BYTES):
    serial_device.initialize_message_size(message_size)

    data = serial_device.read()
    starting_time = datetime.datetime.now()

    while data is not None and serial_device.bytes_received < MAX_BYTES:
        data = serial_device.read()
    print
    print "Finished receiving."

    elapsed_time = datetime.datetime.now() - starting_time
    print serial_device.total_time(elapsed_time)
    print serial_device.throughput(elapsed_time)


def main():
    device = SerialDevice()

    for message_size in range(1000, 250000, 1000):
        run_benchmark(device, message_size)

if __name__ == '__main__':
    main();

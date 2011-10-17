#!/usr/bin/env python

from operator import itemgetter
import datetime
import sys
import string
import usb.core

MAX_BYTES = 10 * 1000 * 10 * 5
STARTING_MESSAGE_SIZE = 12
ENDING_MESSAGE_SIZE = 64
MESSAGE_SIZE_STEP = 12

DATA_ENDPOINT = 0x81
PACKET_SIZE = 27


class MessageDeviceBenchmarker(object):
    def read_all(self):
        data = self.read_one()

        self.bytes_received += self.message_size
        if self.bytes_received % (1000 * 10) == 0:
            print "Received %d kilobytes so far..." % (
                    self.bytes_received / 1000),
            sys.stdout.flush()
            print "\r",
        return data

    def set_message_size(self, message_size):
        self.message_size = message_size
        self.set_message_size_on_device(self.message_size)
        print "Message size switched to %d bytes" % self.message_size
        self.bytes_received = 0


class UsbDevice(MessageDeviceBenchmarker):
    MESSAGE_SIZE_CONTROL_MESSAGE = 0x80

    def __init__(self, vendorId=0x04d8, endpoint=0x81):
        self.vendorId = vendorId
        self.endpoint = endpoint
        self.device = usb.core.find(idVendor=vendorId)
        if not self.device:
            print "Couldn't find a USB device from vendor %s" % self.vendorId
            sys.exit()
        self.reconfigure()

    def set_message_size_on_device(self, message_size):
        self.device.ctrl_transfer(0x40, self.MESSAGE_SIZE_CONTROL_MESSAGE,
                message_size)

    def reconfigure(self):
        self.device.set_configuration()

    def read_one(self):
        return self.device.read(self.endpoint, self.message_size)


def run_benchmark(device, message_size, total_bytes=MAX_BYTES):
    device.set_message_size(message_size)

    data = device.read()
    starting_time = datetime.datetime.now()

    while data is not None and device.bytes_received < MAX_BYTES:
        data = device.read()
        for character in string.ascii_lowercase[:message_size]:
            if character not in data:
                print "Corruption detection on line: %s" % data

    print
    print "Finished receiving."

    elapsed_time = datetime.datetime.now() - starting_time
    throughput = device.throughput(elapsed_time)
    print device.total_time(elapsed_time)
    print "The effective throughput for %d byte messages is %d KB/s" % (
                message_size, throughput)
    return throughput

def main():
    device = UsbDevice()

    results = {}
    for message_size in range(STARTING_MESSAGE_SIZE, ENDING_MESSAGE_SIZE + 1,
            MESSAGE_SIZE_STEP):
        results[message_size] = run_benchmark(device, message_size)

    print
    results = [(key, "%d byte messages -> %d KB/s" % (key, value))
            for key, value in results.iteritems()]

    for result in sorted(results, key=itemgetter(0)):
        print result[1]

if __name__ == '__main__':
    main();

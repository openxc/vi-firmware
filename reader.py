#!/usr/bin/env python

from operator import itemgetter
import datetime
import json
import sys
import argparse
import usb.core
import string


class UsbDevice(object):
    DATA_ENDPOINT = 0x81

    def __init__(self, vendorId=0x04d8, endpoint=0x81, verbose=False):
        self.verbose = verbose
        self.vendorId = vendorId
        self.endpoint = endpoint
        self.message_buffer = ""
        self.messages_received = 0
        self.good_messages = 0

        self.device = usb.core.find(idVendor=vendorId)
        if not self.device:
            print "Couldn't find a USB device from vendor %s" % self.vendorId
            sys.exit()
        self.device.set_configuration()

    def parse_message(self):
        if "\r\n" in self.message_buffer:
            message,_,remainder= self.message_buffer.partition("\r\n")
            try:
                message = json.loads(message)
            except ValueError:
                pass
            else:
                self.good_messages += 1
                return message
            finally:
                self.message_buffer = remainder
                self.messages_received += 1


    def read(self):
        while True:
            self.message_buffer += self.device.read(self.endpoint,
                    128).tostring()
            parsed_message = self.parse_message()

            if (self.messages_received > 0 and
                    self.messages_received % 1000 == 0):
                print "Received %d messages so far (%d%% valid)..." % (
                        self.messages_received,
                        float(self.good_messages) / self.messages_received
                        * 100)

            if parsed_message is not None:
                return parsed_message


    def run(self):
        message = self.read()
        while message is not None:
            message = self.read()
            self.validate(message)

        print
        print "Finished receiving."

    def validate(self, message):
        assert "name" in message
        assert "value" in message
        if self.verbose:
            print message

def parse_options():
    parser = argparse.ArgumentParser(description="Receive and print OpenXC "
        "messages over USB")
    parser.add_argument("--vendor",
            action="store",
            dest="vendor",
            default=0x04d8)
    parser.add_argument("--verbose", "-v",
            action="store_true",
            dest="verbose",
            default=False)

    arguments = parser.parse_args()
    return arguments


def main():
    arguments = parse_options()
    device = UsbDevice(vendorId=arguments.vendor, verbose=arguments.verbose)
    device.run()

if __name__ == '__main__':
    main();

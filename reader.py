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

    def __init__(self, vendorId=0x04d8, endpoint=0x81):
        self.vendorId = vendorId
        self.endpoint = endpoint
        self.message_buffer = ""
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
                print "Bad: %s" % message
                pass
            else:
                print message
                return message
            finally:
                self.message_buffer = remainder


    def read(self):
        while True:
            self.message_buffer += self.device.read(self.endpoint,
                    128).tostring()
            parsed_message = self.parse_message()
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

def parse_options():
    parser = argparse.ArgumentParser(description="Receive and print OpenXC "
        "messages over USB")
    parser.add_argument("--vendor",
            action="store",
            dest="vendor",
            default=0x04d8)

    arguments = parser.parse_args()
    return arguments


def main():
    arguments = parse_options()
    device = UsbDevice(vendorId=arguments.vendor)
    device.run()

if __name__ == '__main__':
    main();

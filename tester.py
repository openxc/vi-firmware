#!/usr/bin/env python

import json
import sys
import argparse
import usb.core
from termcolor import colored

class DataPoint(object):
    BadData = False
    DataPresent = False
    CurrentData = 0.0
    Vocab = []

    def __init__(self, dataname, datatype, datamin=0, datamax=0, vocab=None):
        self.DataName = dataname
        self.DataType = datatype
        self.DataMin = datamin
        self.DataMax = datamax

        # Vocab is a list of acceptable strings for CurrentValue
        self.Vocab = vocab or []

    def NewVal(self, value):
        self.DataPresent = True
        if self.BadData==False:
            self.CurrentData = value
#            print type(value)
            if type(value) != self.DataType:
                self.BadData = True
            else:
                if type(value) is unicode:
                    if value in self.Vocab:
                        self.BadData = False
                    else:
                        self.BadData = True
                elif type(value) is bool:
                    self.BadData = False
                else:
                    if value < self.DataMin:
                        self.BadData = True
                    if value > self.DataMax:
                        self.BadData = True

    def PrintVal(self):
        print self.DataName, '  ',
        if self.DataPresent == False:
            print colored('No Data', 'yellow')
        elif self.BadData == True:
            print colored('Bad Data:  ', 'red'), self.CurrentData
        else:
            print colored('Good Data:  ', 'green'), self.CurrentData


class UsbDevice(object):
    DATA_ENDPOINT = 0x81
    VERSION_CONTROL_COMMAND = 0x80
    RESET_CONTROL_COMMAND = 0x81

    def __init__(self, DataPoints, vendorId=0x04d8, endpoint=0x81,
            verbose=False, dump=False):
        self.verbose = verbose
        self.dump = dump
        self.vendorId = vendorId
        self.endpoint = endpoint
        self.message_buffer = ""
        self.messages_received = 0
        self.good_messages = 0
        self.DataPoints = DataPoints

        self.device = usb.core.find(idVendor=vendorId)
        if not self.device:
            print "Couldn't find a USB device from vendor %s" % self.vendorId
            sys.exit()
        self.device.set_configuration()

    @property
    def version(self):
        raw_version = self.device.ctrl_transfer(0xC0,
                self.VERSION_CONTROL_COMMAND, 0, 0, 10)
        return ''.join([chr(x) for x in raw_version])

    def reset(self):
        self.device.ctrl_transfer(0x40, self.RESET_CONTROL_COMMAND, 0, 0)

    def parse_message(self):
        if "\r\n" in self.message_buffer:
            message,_,remainder= self.message_buffer.partition("\r\n")
            try:
                parsed_message = json.loads(message)
            except ValueError:
                pass
            else:
                self.good_messages += 1
#                print parsed_message['value']
                self.FoundIt = False
                for datapoint in self.DataPoints:
                    if datapoint.DataName == parsed_message['name']:
                        self.FoundIt = True
                        datapoint.NewVal(parsed_message['value'])
                if not self.FoundIt:
                    if self.dump:
                        print message
                return parsed_message
            finally:
                self.message_buffer = remainder
                self.messages_received += 1


    def run(self):
        while True:
            self.message_buffer += self.device.read(self.endpoint,
                    128).tostring()
            self.parse_message()

            if (self.messages_received > 0 and
                    self.messages_received % 1000 == 0):
                print "Received %d messages so far (%d%% valid)..." % (
                        self.messages_received,
                        float(self.good_messages) / self.messages_received
                        * 100)
                for point in self.DataPoints:
                    point.PrintVal()
                print ' '


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
    parser.add_argument("--dump", "-d",
            action="store_true",
            dest="dump",
            default=False)
    parser.add_argument("--version",
            action="store_true",
            dest="version",
            default=False)
    parser.add_argument("--reset",
            action="store_true",
            dest="reset",
            default=False)

    arguments = parser.parse_args()
    return arguments

def Setup_List():
    pointslist = []

    pointslist.append(DataPoint('steering_wheel_angle', float, -360, 360))
    pointslist.append(DataPoint('windshield_wiper_status', bool))
    pointslist.append(DataPoint('brake_pedal_status', bool))
    pointslist.append(DataPoint('parking_brake_status', bool))
    pointslist.append(DataPoint('latitude', float, -90, 90))
    pointslist.append(DataPoint('longitude', float, -180, 180))
    pointslist.append(DataPoint('vehicle_speed', float, 0, 120))
    pointslist.append(DataPoint('odometer', float, 0, 10000))
    pointslist.append(DataPoint('fine_odometer_since_restart', float, 0, 300))
    pointslist.append(DataPoint('engine_speed', float, 0, 300))
    pointslist.append(DataPoint('fuel_level', float, 0, 300))
    pointslist.append(DataPoint('fuel_consumed_since_restart', float, 0, 300))

    pointslist.append(DataPoint('transmission_gear_position', unicode,
            vocab=['first', 'second', 'third', 'fourth', 'fifth', 'sixth',
                'seventh', 'eighth', 'neutral', 'reverse']))
    pointslist.append(DataPoint('ignition_status', unicode,
            vocab=['off', 'accessory', 'run', 'start']))

    for point in pointslist:
        point.PrintVal()
    print ' '

    return pointslist

def main():
    arguments = parse_options()
    DataPoints = Setup_List()

    device = UsbDevice(DataPoints, vendorId=arguments.vendor,
            verbose=arguments.verbose, dump=arguments.dump)
    if arguments.version:
        print "Device is running version %s" % device.version
    elif arguments.reset:
        print "Resetting device..."
        device.reset()
    else:
        device.run()

if __name__ == '__main__':
    main();

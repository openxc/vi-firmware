#!/usr/bin/env python

import json
import sys
import argparse
import usb.core

try:
    from termcolor import colored
except ImportError:
    def colored(text, color):
        return text


class DataPoint(object):
    def __init__(self, name, value_type, min_value=0, max_value=0, vocab=None):
        self.name = name
        self.type = value_type
        self.min_value = min_value
        self.max_value = max_value
        self.event = ''
        self.bad_data = False
        self.current_data = None

        # Vocab is a list of acceptable strings for CurrentValue
        self.vocab = vocab or []

    def update(self, message):
        if self.bad_data:
            # Received bad data at some point - leave it in an error state
            return

        self.current_data = message['value']
        if type(self.current_data) != self.type:
            self.bad_data = True
        else:
            if type(self.current_data) is unicode:
                if self.current_data in self.vocab:
                    if len(message) > 2:
                        self.event = message['event']
                else:
                    self.bad_data = True
            else:
                if self.current_data < self.min_value:
                    self.bad_data = True
                elif self.current_data > self.max_value:
                    self.bad_data = True

    def __str__(self):
        result = self.name + "  "
        if not self.current_data:
            result += colored('No Data', 'yellow')
        else:
            if self.bad_data:
                result += colored('Bad Data:  ', 'red')
            else:
                result += colored('Good Data:  ', 'green')
            result += self.current_data +  ' ' + self.event
        return result


class UsbDevice(object):
    DATA_ENDPOINT = 0x81
    VERSION_CONTROL_COMMAND = 0x80
    RESET_CONTROL_COMMAND = 0x81

    def __init__(self, vendorId=0x04d8, endpoint=0x81, verbose=False,
            dump=False, dashboard=False, elements=None):
        self.verbose = verbose
        self.dump = dump
        self.dashboard = dashboard
        self.vendorId = vendorId
        self.endpoint = endpoint
        self.message_buffer = ""
        self.messages_received = 0
        self.good_messages = 0
        self.elements = elements or []

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
                if self.dump:
                    print message
                if self.verbose:
                    print parsed_message
                if self.dashboard:
                    for element in self.elements:
                        if element.name == parsed_message.get('name', None):
                            element.update(parsed_message)
                            break
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
                for element in self.elements:
                    print element
                print


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
    parser.add_argument("--dashboard",
            action="store_true",
            dest="dashboard",
            default=False)

    arguments = parser.parse_args()
    return arguments


def initialize_elements():
    elements = []

    elements.append(DataPoint('steering_wheel_angle', float, -460, 460))
    elements.append(DataPoint('engine_speed', float, 0, 8000))
    elements.append(DataPoint('transmission_gear_position', unicode,
        vocab=['first', 'second', 'third', 'fourth', 'fifth', 'sixth',
            'seventh', 'eighth', 'neutral', 'reverse']))
    elements.append(DataPoint('ignition_status', unicode,
        vocab=['off', 'accessory', 'run', 'start']))
    elements.append(DataPoint('brake_pedal_status', bool))
    elements.append(DataPoint('parking_brake_status', bool))
    elements.append(DataPoint('headlamp_status', bool))
    elements.append(DataPoint('accelerator_pedal_position', float, 0, 300))
    elements.append(DataPoint('powertrain_torque', float, -100, 300))
    elements.append(DataPoint('vehicle_speed', float, 0, 120))
    elements.append(DataPoint('fuel_consumed_since_restart', float, 0, 300))
    elements.append(DataPoint('fine_odometer_since_restart', float, 0, 300))
    elements.append(DataPoint('door_status', unicode,
        vocab=['driver', 'rear_right', 'rear_left', 'passenger']))
    elements.append(DataPoint('windshield_wiper_status', bool))
    elements.append(DataPoint('odometer', float, 0, 10000))
    elements.append(DataPoint('high_beam_status', bool))
    elements.append(DataPoint('fuel_level', float, 0, 300))
    elements.append(DataPoint('latitude', float, -90, 90))
    elements.append(DataPoint('longitude', float, -180, 180))

    for element in elements:
        print element
    print

    return elements


def main():
    arguments = parse_options()

    device = UsbDevice(vendorId=arguments.vendor, verbose=arguments.verbose,
            dump=arguments.dump, dashboard=arguments.dashboard,
            elements=initialize_elements())
    if arguments.version:
        print "Device is running version %s" % device.version
    elif arguments.reset:
        print "Resetting device..."
        device.reset()
    else:
        device.run()

if __name__ == '__main__':
    main();

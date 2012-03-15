#!/usr/bin/env python

import json
import sys
import argparse
import usb.core
import curses
import curses.wrapper


class DataPoint(object):
    def __init__(self, name, value_type, min_value=0, max_value=0, vocab=None):
        self.name = name
        self.type = value_type
        self.min_value = min_value
        self.max_value = max_value
        self.range = max_value - min_value
        if self.range <= 0:
            self.range = 1
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
            if type(self.current_data) is bool:
                return
            elif type(self.current_data) is unicode:
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

    def print_to_window(self, window, row):
        window.addstr(row, 0, self.name)
        result = ""
        if self.current_data is None:
            window.addstr(row, 30, "No Data", curses.color_pair(3))
        else:
            if self.bad_data:
                window.addstr(row, 30, "Bad", curses.color_pair(1))
            else:
                window.addstr(row, 30, "Good", curses.color_pair(2))
                if self.type == float:
                    percent = self.current_data - self.min_value
                    percent /= self.range
                    Count = 0
                    graph = "*"
                    percent -= .1
                    while percent > 0:
                        graph += "-"
                        Count += 1
                        percent -= .1
                    graph += "|"
                    Count += 1
                    while Count < 10:
                        graph += "-"
                        Count +=1
                    graph += "* "
                    window.addstr(row, 40, graph)
            window.addstr(row, 55, str(self.current_data) + " " +
                    str(self.event))


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

    def run(self, window=None):
        if window is not None:
            curses.use_default_colors()
            curses.init_pair(1, curses.COLOR_RED, -1)
            curses.init_pair(2, curses.COLOR_GREEN, -1)
            curses.init_pair(3, curses.COLOR_YELLOW, -1)

        while True:
            self.message_buffer += self.device.read(self.endpoint,
                    128).tostring()
            self.parse_message()

            if window is not None:
                window.addstr(len(self.elements), 0,
                        "Received %d messages so far (%d%% valid)..." % (
                        self.messages_received,
                        float(self.good_messages) / self.messages_received
                        * 100), curses.A_REVERSE)
            if self.dashboard and window is not None:
                for row, element in enumerate(self.elements):
                    element.print_to_window(window, row)
                window.refresh()
                window.clear()

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


def initialize_elements(dashboard):
    elements = []

    elements.append(DataPoint('steering_wheel_angle', float, -600, 600))
    elements.append(DataPoint('engine_speed', float, 0, 8000))
    elements.append(DataPoint('transmission_gear_position', unicode,
        vocab=['first', 'second', 'third', 'fourth', 'fifth', 'sixth',
            'seventh', 'eighth', 'neutral', 'reverse', 'park']))
    elements.append(DataPoint('ignition_status', unicode,
        vocab=['off', 'accessory', 'run', 'start']))
    elements.append(DataPoint('brake_pedal_status', bool))
    elements.append(DataPoint('parking_brake_status', bool))
    elements.append(DataPoint('headlamp_status', bool))
    elements.append(DataPoint('accelerator_pedal_position', float, 0, 200))
    elements.append(DataPoint('powertrain_torque', float, -100, 300))
    elements.append(DataPoint('vehicle_speed', float, 0, 120))
    elements.append(DataPoint('fuel_consumed_since_restart', float, 0, 300))
    elements.append(DataPoint('fine_odometer_since_restart', float, 0, 300))
    elements.append(DataPoint('door_status', unicode,
        vocab=['driver', 'rear_left', 'rear_right', 'passenger']))
    elements.append(DataPoint('windshield_wiper_status', bool))
    elements.append(DataPoint('odometer', float, 0, 100000))
    elements.append(DataPoint('high_beam_status', bool))
    elements.append(DataPoint('fuel_level', float, 0, 300))
    elements.append(DataPoint('latitude', float, -90, 90))
    elements.append(DataPoint('longitude', float, -180, 180))

    return elements


def main():
    arguments = parse_options()

    device = UsbDevice(vendorId=arguments.vendor, verbose=arguments.verbose,
            dump=arguments.dump, dashboard=arguments.dashboard,
            elements=initialize_elements(arguments.dashboard))
    if arguments.version:
        print "Device is running version %s" % device.version
    elif arguments.reset:
        print "Resetting device..."
        device.reset()
    elif arguments.dashboard:
        curses.wrapper(device.run)
    else:
        device.run()

if __name__ == '__main__':
    main()

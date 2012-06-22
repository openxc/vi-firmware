#!/usr/bin/env python

import json
import sys
import argparse
import usb.core
import serial
import curses
import curses.wrapper
import time
from datetime import datetime


class DataPoint(object):
    def __init__(self, name, value_type, min_value=0, max_value=0, vocab=None,
            events=False, messages_received=0):
        self.name = name
        self.type = value_type
        self.min_value = min_value
        self.max_value = max_value
        self.range = max_value - min_value
        if self.range <= 0:
            self.range = 1
        self.event = ''
        self.bad_data = False
        self.bad_data_tally = 0
        self.current_data = None
        self.events_active = events
        self.events = []
        self.messages_received = messages_received
        self.rate_messages_received = 0

        # Vocab is a list of acceptable strings for CurrentValue
        self.vocab = vocab or []

        if self.events_active is True:
            for x in range(len(self.vocab)):
                self.events.append("")

    def update(self, message):
        if self.bad_data:
            self.bad_data_tally += 1
            self.bad_data = False

        self.rate_messages_received += 1
        self.messages_received += 1
        self.current_data = message.get('value', None)
        if type(self.current_data) == int:
            self.current_data = float(self.current_data)
        if type(self.current_data) != self.type:
            self.bad_data = True
        else:
            if type(self.current_data) is bool:
                return
            elif type(self.current_data) is unicode:
                if self.current_data in self.vocab:
                    #Save the event in the proper spot.
                    if (len(message) > 2) and (self.events_active is True):
                        self.events[self.vocab.index(self.current_data)
                                ] = message['event']
                else:
                    self.bad_data = True
            else:
                if self.current_data < self.min_value:
                    self.bad_data = True
                elif self.current_data > self.max_value:
                    self.bad_data = True

    def print_to_window(self, window, row, begin_time):
        window.addstr(row, 0, self.name)
        result = ""
        if self.current_data is not None and not self.bad_data:
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
                window.addstr(row, 30, graph)

            if self.events_active is False:
                window.addstr(row, 45, str(self.current_data))
            else:
                result = ""
                for item, value in enumerate(self.vocab):
                    if value == "driver":
                        keyword = "dr"
                    elif value == "passenger":
                        keyword = "ps"
                    elif value == "rear_right":
                        keyword = "rr"
                    elif value == "rear_left":
                        keyword = "rl"
                result = (result + keyword + ":" + str(self.events[item]) + " ")
                window.addstr(row, 45, result)

            if self.bad_data_tally > 0:
                bad_data_color = curses.color_pair(1)
            else:
                bad_data_color = curses.color_pair(2)

            window.addstr(row, 80, "Errors: " + str(self.bad_data_tally),
                    bad_data_color)
            window.addstr(row, 95, "Messages: " + str(self.messages_received))
            window.addstr(row, 110, "Frequency: " +
                    str(int(self.rate_messages_received /
                        ((datetime.now() - begin_time).total_seconds()))))


class CanTranslator(object):
    def __init__(self, verbose=False, dump=False, dashboard=False,
            elements=None):
        self.verbose = verbose
        self.dump = dump
        self.dashboard = dashboard
        self.message_buffer = ""
        self.messages_received = 0
        self.good_messages = 0
        self.elements = elements or []
        self.total_bytes_received = 0
        self.rate_bytes_received = 0
        self.begin_time = datetime.now()

    def parse_message(self):
        if "\n" in self.message_buffer:
            message,_,remainder= self.message_buffer.partition("\n")
            try:
                parsed_message = json.loads(message)
            except ValueError:
                pass
            else:
                self.good_messages += 1
                if self.dump:
                    print "%f: %s" % (time.time(), message)
                if self.verbose:
                    print parsed_message
                if self.dashboard:
                    self.total_bytes_received += sys.getsizeof(parsed_message)
                self.rate_bytes_received += sys.getsizeof(parsed_message)
                for element in self.elements:
                    if element.name == parsed_message.get('name', None):
                        element.update(parsed_message)
                        break
                return parsed_message
            finally:
                self.message_buffer = remainder
                self.messages_received += 1

    # This resets the number of bytes received for both individual messages and
    # the total number of messages and also resets begin_time to set up a time
    # frame of 10 seconds in which to calculate the data rates
    def date_rate_management(self):
        if ((datetime.now() - self.begin_time).total_seconds() > 10):
            self.begin_time = datetime.now()
            self.rate_bytes_received = 0;
            for element in self.elements:
                    element.rate_messages_received = 0

    def run(self, window=None):
        if window is not None:
            curses.use_default_colors()
            curses.init_pair(1, curses.COLOR_RED, -1)
            curses.init_pair(2, curses.COLOR_GREEN, -1)
            curses.init_pair(3, curses.COLOR_YELLOW, -1)

        while True:
            self.message_buffer += self.read()
            self.parse_message()
            self.date_rate_management()

            if self.dashboard and window is not None:
                window.erase()
                for row, element in enumerate(self.elements):
                    element.print_to_window(window, row, self.begin_time)
                percentage_good = 0
                if self.messages_received != 0:
                    percentage_good = (self.good_messages /
                            self.messages_received)
                window.addstr(len(self.elements), 0,
                        "Received %d messages so far (%d%% valid)..." % (
                        self.messages_received, percentage_good * 100),
                        curses.A_REVERSE)
                window.addstr(len(self.elements) + 1, 0,
                        "Total Bytes Received: " +
                        str(self.total_bytes_received), curses.A_REVERSE)
                window.addstr(len(self.elements) + 2, 0, "Overall Data Rate: " +
                    str((self.rate_bytes_received) / (datetime.now()
                    - self.begin_time).total_seconds()) + " Bps",
                     curses.A_REVERSE)
                window.refresh()


class SerialCanTransaltor(CanTranslator):
    def __init__(self, port="/dev/ttyUSB0", baud_rate=115200, verbose=False,
            dump=False, dashboard=False, elements=None):
        super(SerialCanTransaltor, self).__init__(verbose, dump, dashboard,
                elements)
        self.port = port
        self.baud_rate = baud_rate
        self.device = serial.Serial(self.port, self.baud_rate)

    def read(self):
        return self.device.readline()


class UsbCanTranslator(CanTranslator):
    INTERFACE = 0
    VERSION_CONTROL_COMMAND = 0x80
    RESET_CONTROL_COMMAND = 0x81

    def __init__(self, vendor_id=0x04d8, verbose=False, dump=False,
            dashboard=False, elements=None):
        super(UsbCanTranslator, self).__init__(verbose, dump, dashboard,
                elements)
        self.vendor_id = vendor_id

        self.device = usb.core.find(idVendor=vendor_id)
        if not self.device:
            print "Couldn't find a USB device from vendor %s" % self.vendor_id
            sys.exit()
        self.device.set_configuration()
        config = self.device.get_active_configuration()
        interface_number = config[(0,0)].bInterfaceNumber
        interface = usb.util.find_descriptor(config,
                bInterfaceNumber=interface_number)

        self.out_endpoint = usb.util.find_descriptor(interface,
                custom_match = \
                        lambda e: \
                        usb.util.endpoint_direction(e.bEndpointAddress) == \
                        usb.util.ENDPOINT_OUT)
        self.in_endpoint = usb.util.find_descriptor(interface,
                custom_match = \
                        lambda e: \
                        usb.util.endpoint_direction(e.bEndpointAddress) == \
                        usb.util.ENDPOINT_IN)

        if not self.out_endpoint or not self.in_endpoint:
            print "Couldn't find proper endpoints on the USB device"
            sys.exit()

    def read(self):
        return self.in_endpoint.read(64, 1000000).tostring()

    @property
    def version(self):
        raw_version = self.device.ctrl_transfer(0xC0,
                self.VERSION_CONTROL_COMMAND, 0, 0, 100)
        return ''.join([chr(x) for x in raw_version])

    def reset(self):
        self.device.ctrl_transfer(0x40, self.RESET_CONTROL_COMMAND, 0, 0)

    def write(self, name, value):
        if value == "true":
            value = True
        elif value == "false":
            value = False
        else:
            try:
                value = float(value)
            except ValueError:
                pass

        message = json.dumps({'name': name, 'value': value})
        bytes_written = self.out_endpoint.write(message + "\x00")
        assert bytes_written == len(message) + 1

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
    parser.add_argument("--serial", "-s",
            action="store_true",
            dest="serial",
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
    parser.add_argument("--write", "-w",
            nargs=2,
            action="store",
            dest="write")

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
    elements.append(DataPoint('accelerator_pedal_position', float, 0, 100))
    elements.append(DataPoint('torque_at_transmission', float, -400, 1500))
    elements.append(DataPoint('vehicle_speed', float, 0, 120))
    elements.append(DataPoint('fuel_consumed_since_restart', float, 0, 300))
    elements.append(DataPoint('fine_odometer_since_restart', float, 0, 300))
    elements.append(DataPoint('door_status', unicode,
        vocab=['driver', 'rear_left', 'rear_right', 'passenger'], events=True))
    elements.append(DataPoint('windshield_wiper_status', bool))
    elements.append(DataPoint('odometer', float, 0, 100000))
    elements.append(DataPoint('high_beam_status', bool))
    elements.append(DataPoint('fuel_level', float, 0, 300))
    elements.append(DataPoint('latitude', float, -90, 90))
    elements.append(DataPoint('longitude', float, -180, 180))
    elements.append(DataPoint('heater_status', bool))
    elements.append(DataPoint('air_conditioning_status', bool))
    elements.append(DataPoint('charging_status', bool))
    elements.append(DataPoint('range', float, 0, 500))
    elements.append(DataPoint('gear_lever_position', unicode,
        vocab=['first', 'second', 'third', 'fourth', 'fifth', 'sixth',
            'seventh', 'neutral', 'reverse', 'park', 'drive', 'low', 'sport']))
    elements.append(DataPoint('battery_level', float, 0, 100))
    elements.append(DataPoint('cabin_temperature', float, -50, 150))

    return elements


def main():
    arguments = parse_options()

    if arguments.serial:
        device_class = SerialCanTransaltor
        kwargs = dict()
    else:
        device_class = UsbCanTranslator
        kwargs = dict(vendor_id=arguments.vendor)

    device = device_class(verbose=arguments.verbose, dump=arguments.dump,
            dashboard=arguments.dashboard,
            elements=initialize_elements(arguments.dashboard), **kwargs)
    if arguments.version:
        print "Device is running version %s" % device.version
    elif arguments.reset:
        print "Resetting device..."
        device.reset()
    elif arguments.dashboard:
        curses.wrapper(device.run)
    elif arguments.write:
        device.write(arguments.write[0], arguments.write[1])
    else:
        device.run()

if __name__ == '__main__':
    main()

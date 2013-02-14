#!/usr/bin/env python

from __future__ import print_function
import collections
import itertools
import operator
from collections import defaultdict
import sys
import argparse


# Only works with 2 CAN buses since we are limited by 2 CAN controllers,
# and we want to be a little careful that we always expect 0x101 to be
# plugged into the CAN1 controller and 0x102 into CAN2.
VALID_BUS_ADDRESSES = ("0x101", "0x102")

def fatal_error(message):
    sys.stderr.write("ERROR: %s\n" % message)
    sys.exit(1)

def parse_options():
    parser = argparse.ArgumentParser(description="Generate C source code from "
            "CAN signal descriptions in JSON")
    json_files = parser.add_argument("-j", "--json",
            action="append",
        type=str,
        nargs='*',
            dest="json_files",
            metavar="FILE",
            help="generate source from this JSON file")
    parser.add_argument("-m", "--message-set",
            action="store", type=str, dest="message_set", metavar="MESSAGE_SET",
            default="generic", help="name of the vehicle or platform")

    arguments = parser.parse_args()

    if not arguments.json_files:
        raise argparse.ArgumentError(json_files,
                "Must specify at least one JSON file.")

    return arguments


def quacks_like_dict(object):
    """Check if object is dict-like"""
    return isinstance(object, collections.Mapping)


def merge(a, b):
    """Merge two deep dicts non-destructively

    Uses a stack to avoid maximum recursion depth exceptions

    >>> a = {'a': 1, 'b': {1: 1, 2: 2}, 'd': 6}
    >>> b = {'c': 3, 'b': {2: 7}, 'd': {'z': [1, 2, 3]}}
    >>> c = merge(a, b)
    >>> from pprint import pprint; pprint(c)
    {'a': 1, 'b': {1: 1, 2: 7}, 'c': 3, 'd': {'z': [1, 2, 3]}}
    """
    assert quacks_like_dict(a), quacks_like_dict(b)
    dst = a.copy()

    stack = [(dst, b)]
    while stack:
        current_dst, current_src = stack.pop()
        for key in current_src:
            if key not in current_dst:
                current_dst[key] = current_src[key]
            else:
                if (quacks_like_dict(current_src[key]) and
                        quacks_like_dict(current_dst[key])):
                    stack.append((current_dst[key], current_src[key]))
                else:
                    current_dst[key] = current_src[key]
    return dst


class Command(object):
    def __init__(self, generic_name, handler=None):
        self.generic_name = generic_name
        self.handler = handler

    def __str__(self):
        return "{ \"%s\", %s }," % (self.generic_name, self.handler)


class Message(object):
    def __init__(self, buses, bus_address, id, name, handler=None):
        self.bus_address = bus_address
        self.buses = buses
        self.id = int(id, 0)
        self.name = name
        self.handler = handler
        self.signals = []

    def __str__(self):
        return "{&CAN_BUSES[%d], %d}, // %s" % (
                self._lookupBusIndex(self.buses, self.bus_address),
                self.id, self.name)

    @staticmethod
    def _lookupBusIndex(buses, bus_address):
        for bus_number, candidate_bus_address in enumerate(VALID_BUS_ADDRESSES):
            if candidate_bus_address == bus_address:
                return bus_number
        fatal_error("Bus address %s is invalid, only %s and %s are allowed" %
                (bus_address, VALID_BUS_ADDRESSES[0], VALID_BUS_ADDRESSES[1]))


class Signal(object):
    def __init__(self, messages=None, message=None, name=None,
            generic_name=None, position=None, length=None, factor=1, offset=0,
            min_value=0.0, max_value=0.0, handler=None, ignore=False,
            states=None, send_frequency=0, send_same=True,
            writable=False, write_handler=None):
        self.messages = messages
        self.message = message
        self.name = name
        self.generic_name = generic_name
        self.position = position
        self.length = length
        self.factor = factor
        self.offset = offset
        self.min_value = min_value
        self.max_value = max_value
        self.handler = handler
        self.writable = writable
        self.write_handler = write_handler
        if ignore:
            self.handler = "ignoreHandler"
        self.array_index = 0
        # the frequency determines how often the message should be propagated. a
        # frequency of 1 means that every time the signal it is received we will
        # try to handle it. a frequency of 2 means that every other signal
        # will be handled (and the other half is ignored). This is useful for
        # trimming down the data rate of the stream over USB.
        self.send_frequency = send_frequency
        self.send_same = send_same
        self.states = states or []
        if len(self.states) > 0 and self.handler is None:
            self.handler = "stateHandler"

    # Construct a Signal instance from an XML node exported from a Vector CANoe
    # .dbc file.
    @classmethod
    def from_xml_node(cls, node):
        signal = Signal(name=node.find("Name").text,
                position=int(node.find("Bitposition").text),
                length=int(node.find("Bitsize").text),
                factor=float(node.find("Factor").text),
                offset=float(node.find("Offset").text),
                min_value=float(node.find("Minimum").text),
                max_value=float(node.find("Maximum").text))

        # Invert the bit index to match the Excel mapping.
        signal.position = Signal._invert_bit_index(signal.position,
                signal.length)
        return signal

    def to_dict(self):
        return {"generic_name": self.generic_name,
                "bit_position": self.position,
                "bit_size": self.length,
                "factor": self.factor,
                "offset": self.offset,
                "min_value": self.min_value,
                "max_value": self.max_value}

    def validate(self):
        if self.position == None:
            sys.stderr.write("ERROR: %s (generic name: %s) is incomplete\n" % (
                self.name, self.generic_name))
            return False
        return True

    @classmethod
    def _invert_bit_index(cls, i, l):
        (b, r) = divmod(i, 8)
        end = (8 * b) + (7 - r)
        return(end - l + 1)

    @staticmethod
    def _lookupMessageIndex(messages, message):
        for i, candidate in enumerate(messages):
            if candidate.id == message.id:
                return i

    def __str__(self):
        result =  ("{&CAN_MESSAGES[%d], \"%s\", %s, %d, %f, %f, %f, %f, "
                    "%d, %s, false, " % (
                self._lookupMessageIndex(self.messages, self.message),
                self.generic_name, self.position, self.length, self.factor,
                self.offset, self.min_value, self.max_value,
                self.send_frequency, str(self.send_same).lower()))
        if len(self.states) > 0:
            result += "SIGNAL_STATES[%d], %d" % (self.states_index,
                    len(self.states))
        else:
            result += "NULL, 0"
        result += ", %s, %s" % (str(self.writable).lower(),
                self.write_handler or "NULL")
        result += "}, // %s" % self.name
        return result


class SignalState(object):
    def __init__(self, value, name):
        self.value = value
        self.name = name

    def __str__(self):
        return "{%d, \"%s\"}" % (self.value, self.name)


class Parser(object):
    def __init__(self, name=None):
        self.name = name
        self.buses = defaultdict(dict)
        self.signal_count = 0
        self.message_count = 0
        self.command_count = 0

    def parse(self):
        raise NotImplementedError

    def print_header(self):
        print("#ifndef CAN_EMULATOR")
        print("#include \"canread.h\"")
        print("#include \"canwrite.h\"")
        print("#include \"signals.h\"")
        print("#include \"log.h\"")
        if getattr(self, 'uses_custom_handlers', None):
            print("#include \"shared_handlers.h\"")
            print("#include \"handlers.h\"")
        print()
        print("extern Listener listener;")
        print()
        print("#ifdef __LPC17XX__")
        print("#define can1 LPC_CAN1")
        print("#define can2 LPC_CAN2")
        print("#endif // __LPC17XX__")
        print()
        print("#ifdef __PIC32__")
        print("extern void* can1;")
        print("extern void* can2;")
        print("extern void handleCan1Interrupt();")
        print("extern void handleCan2Interrupt();")
        print("#endif // __PIC32__")
        print()

    def validate_messages(self):
        valid = True
        for bus in list(self.buses.values()):
            for message in bus['messages']:
                if message.handler is not None:
                    self.uses_custom_handlers = True
                for signal in message.signals:
                    valid = valid and signal.validate()
                    if signal.handler is not None:
                        self.uses_custom_handlers = True
        return valid

    def validate_name(self):
        if self.name is None:
            sys.stderr.write("ERROR: missing message set (%s)" % self.name)
            return False
        return True

    def _print_bus_struct(self, bus_address, bus, bus_number):
        print("    { %d, %s, can%d, " % (bus['speed'], bus_address, bus_number))
        print("#ifdef __PIC32__")
        print("        handleCan%dInterrupt," % bus_number)
        print("#endif // __PIC32__")
        print("    },")

    def print_source(self):
        if not self.validate_messages() or not self.validate_name():
            fatal_error("unable to generate code")
        self.print_header()

        print("const int CAN_BUS_COUNT = %d;" % len(self.buses))
        print("CanBus CAN_BUSES[CAN_BUS_COUNT] = {")
        for bus_number, bus_address in enumerate(VALID_BUS_ADDRESSES):
            bus = self.buses.get(bus_address, None)
            if bus is not None:
                self._print_bus_struct(bus_address, bus, bus_number + 1)

        print("};")
        print()

        print("const int MESSAGE_COUNT = %d;" % self.message_count)
        print("CanMessage CAN_MESSAGES[MESSAGE_COUNT] = {")

        i = 1
        for bus in list(self.buses.values()):
            for message in bus['messages']:
                message.array_index = i - 1
                print("    %s" % message)
                i += 1
        print("};")
        print()

        print("const int SIGNAL_COUNT = %d;" % self.signal_count)
        # TODO need to handle signals with more than 12 states
        print("CanSignalState SIGNAL_STATES[SIGNAL_COUNT][%d] = {" % 12)

        states_index = 0
        for bus in list(self.buses.values()):
            for message in bus['messages']:
                for signal in message.signals:
                    if len(signal.states) > 0:
                        print("    {", end=' ')
                        for state in signal.states:
                            print("%s," % state, end=' ')
                        print("},")
                        signal.states_index = states_index
                        states_index += 1
        print("};")
        print()

        print("CanSignal SIGNALS[SIGNAL_COUNT] = {")

        i = 1
        for bus in list(self.buses.values()):
            for message in bus['messages']:
                message.signals = sorted(message.signals,
                        key=operator.attrgetter('generic_name'))
                for signal in message.signals:
                    signal.array_index = i - 1
                    print("    %s" % signal)
                    i += 1
        print("};")
        print()

        print("const int COMMAND_COUNT = %d;" % self.command_count)
        print("CanCommand COMMANDS[COMMAND_COUNT] = {")

        for command in self.commands:
            print("    ", command)

        print("};")
        print()

        # TODO store all of this in a separate, committed .cpp file
        print("CanCommand* getCommands() {")
        print("    return COMMANDS;")
        print("}")
        print()

        print("int getCommandCount() {")
        print("    return COMMAND_COUNT;")
        print("}")
        print()

        print("CanSignal* getSignals() {")
        print("    return SIGNALS;")
        print("}")
        print()

        print("int getSignalCount() {")
        print("    return SIGNAL_COUNT;")
        print("}")
        print()

        print("CanBus* getCanBuses() {")
        print("    return CAN_BUSES;")
        print("}")
        print()

        print("int getCanBusCount() {")
        print("    return CAN_BUS_COUNT;")
        print("}")
        print()

        print("const char* getMessageSet() {")
        print("    return \"%s\";" % self.name)
        print("}")
        print()

        print("void decodeCanMessage(CanBus* bus, int id, uint64_t data) {")
        print("    switch(bus->address) {")
        for bus_address, bus in self.buses.items():
            print("    case %s:" % bus_address)
            print("        switch (id) {")
            for message in bus['messages']:
                print("        case 0x%x: // %s" % (message.id, message.name))
                if message.handler is not None:
                    print(("            %s(id, data, SIGNALS, " %
                        message.handler + "SIGNAL_COUNT, &listener);"))
                for signal in (s for s in message.signals):
                    if signal.handler:
                        print(("            translateCanSignal(&listener, "
                                "&SIGNALS[%d], data, " % signal.array_index +
                                "&%s, SIGNALS, SIGNAL_COUNT); // %s" % (
                                signal.handler, signal.name)))
                    else:
                        print(("            translateCanSignal(&listener, "
                                "&SIGNALS[%d], " % signal.array_index +
                                "data, SIGNALS, SIGNAL_COUNT); // %s"
                                    % signal.name))
                print("            break;")
            print("        }")
        print("    }")

        if self._message_count() == 0:
            print("    passthroughCanMessage(&listener, id, data);")

        print("}\n")

        # Create a set of filters.
        self.print_filters()
        print()
        print("#endif // CAN_EMULATOR")

    def _message_count(self):
        return sum((len(bus['messages']) for bus in list(self.buses.values())))

    def print_filters(self):
        # These arrays can't be initialized when we create the variables or else
        # they end up in the .data portion of the compiled program, and it
        # becomes too big for the microcontroller. Initializing them at runtime
        # gets around that problem.
        print("CanFilter FILTERS[%d];" % self._message_count())

        print()
        print("CanFilter* initializeFilters(uint64_t address, int* count) {")
        print("    switch(address) {")
        for bus_address, bus in self.buses.items():
            print("    case %s:" % bus_address)
            print("        *count = %d;" % len(bus['messages']))
            for i, message in enumerate(bus['messages']):
                print("        FILTERS[%d] = {%d, 0x%x, %d};" % (
                        i, i, message.id, 1))
            print("        break;")
        print("    }")
        print("    return FILTERS;")
        print("}")


class JsonParser(Parser):
    def __init__(self, filenames, name=None):
        super(JsonParser, self).__init__(name)
        if not isinstance(filenames, list):
            filenames = [filenames]
        else:
            filenames = itertools.chain(*filenames)
        self.json_files = filenames

    # The JSON parser accepts the format specified in the README.
    def parse(self):
        import json
        merged_dict = {}
        for filename in self.json_files:
            with open(filename) as json_file:
                try:
                    data = json.load(json_file)
                except ValueError as e:
                    fatal_error("%s does not contain valid JSON: \n%s\n" %
                            (filename, e))
                merged_dict = merge(merged_dict, data)

        self.commands = []
        for bus_address, bus_data in merged_dict.items():
            speed = bus_data.get('speed', None)
            if speed is None:
                fatal_error("Bus %s is missing the 'speed' attribute" %
                        bus_address)
                sys.exit(1)
            self.buses[bus_address]['speed'] = speed
            self.buses[bus_address].setdefault('messages', [])
            for command_id, command_data in bus_data.get(
                    'commands', {}).items():
                self.command_count += 1
                command = Command(command_id, command_data.get('handler', None))
                self.commands.append(command)

            for message_id, message_data in bus_data.get('messages', {}
                    ).items():
                self.signal_count += len(message_data['signals'])
                self.message_count += 1
                message = Message(self.buses, bus_address, message_id,
                        message_data.get('name', None),
                        message_data.get('handler', None))
                for signal_name, signal in message_data['signals'].items():
                    states = []
                    for name, raw_matches in signal.get('states', {}).items():
                        for raw_match in raw_matches:
                            states.append(SignalState(raw_match, name))
                    message.signals.append(Signal(
                            self.buses[bus_address]['messages'],
                            message,
                            signal_name,
                            signal.get('generic_name', None),
                            signal.get('bit_position', None),
                            signal.get('bit_size', None),
                            signal.get('factor', 1.0),
                            signal.get('offset', 0.0),
                            signal.get('min_value', 0.0),
                            signal.get('max_value', 0.0),
                            signal.get('value_handler', None),
                            signal.get('ignore', False),
                            states,
                            signal.get('send_frequency', 1),
                            signal.get('send_same', True),
                            signal.get('writable', False),
                            signal.get('write_handler', None)))
                self.buses[bus_address]['messages'].append(message)

def main():
    arguments = parse_options()

    parser = JsonParser(arguments.json_files, arguments.message_set)

    parser.parse()
    parser.print_source()

if __name__ == "__main__":
    sys.exit(main())

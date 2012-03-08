#!/usr/bin/env python

import collections
import itertools
from collections import defaultdict
import sys
import argparse

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
                if quacks_like_dict(current_src[key]) and quacks_like_dict(current_dst[key]) :
                    stack.append((current_dst[key], current_src[key]))
                else:
                    current_dst[key] = current_src[key]
    return dst

class Message(object):
    def __init__(self, id, name, handler=None):
        self.id = id
        self.name = name
        self.handler = handler
        self.signals = []


class Signal(object):
    def __init__(self, id=None, name=None, generic_name=None, position=None,
            length=None, factor=1, offset=0, min_value=0, max_value=0,
            handler=None, ignore=False, states=None):
        self.id = id
        self.name = name
        self.generic_name = generic_name
        self.position = position
        self.length = length
        self.factor = factor
        self.offset = offset
        self.min_value = min_value
        self.max_value = max_value
        self.handler = handler
        self.ignore = ignore
        self.array_index = 0
        self.states = states or []
        if len(self.states) > 0 and self.handler is None:
            self.handler = "char* stateHandler"

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
            sys.stderr.write("ERROR: %s is incomplete\n" % self.generic_name)
            return False
        return True

    @classmethod
    def _invert_bit_index(cls, i, l):
        (b, r) = divmod(i, 8)
        end = (8 * b) + (7 - r)
        return(end - l + 1)

    def __str__(self):
        result =  "{%d, \"%s\", %s, %d, %f, %f, %f, %f" % (
                self.id, self.generic_name, self.position, self.length,
                self.factor, self.offset, self.min_value, self.max_value)
        if len(self.states) > 0:
            result += ", SIGNAL_STATES[%d], %d" % (self.states_index,
                    len(self.states))
        result += "}, // %s" % self.name
        return result


class SignalState(object):
    def __init__(self, value, name):
        self.value = value
        self.name = name

    def __str__(self):
        return "{%d, \"%s\"}" % (self.value, self.name)


class Parser(object):
    def __init__(self):
        self.buses = defaultdict(list)
        self.signal_count = 0

    def parse(self):
        raise NotImplementedError

    def print_header(self):
        print "#include \"canutil.h\"\n"
        print "extern USBDevice usbDevice;\n"

    def validate_messages(self):
        valid = True
        for bus in self.buses.values():
            for message in bus:
                for signal in message.signals:
                    valid = valid and signal.validate()
        return valid

    def print_source(self):
        if not self.validate_messages():
            sys.exit(1)
        self.print_header()
        # TODO need to handle signals with more than 10 states
        print "CanSignalState SIGNAL_STATES[%d][%d] = {" % (
                self.signal_count, 10)

        states_index = 0
        for bus in self.buses.values():
            for message in bus:
                for signal in message.signals:
                    if len(signal.states) > 0:
                        print "    {",
                        for state in signal.states:
                            print "%s," % state,
                        print "},"
                        signal.states_index = states_index
                        states_index += 1
        print "};"
        print

        print "int SIGNAL_COUNT = %d;" % self.signal_count
        print "CanSignal SIGNALS[%d] = {" % self.signal_count

        i = 1
        for bus in self.buses.values():
            for message in bus:
                for signal in message.signals:
                    signal.array_index = i - 1
                    print "    %s" % signal
                    i += 1
        print "};"
        print

        print "void decodeCanMessage(int id, uint8_t* data) {"
        print "    switch (id) {"
        for bus in self.buses.values():
            for message in bus:
                print "    case 0x%x: // %s" % (message.id, message.name)
                if message.handler is not None:
                    print ("        extern void %s(int, uint8_t*, CanSignal*, int, USBDevice* usbDevice);"
                            % message.handler)
                    print ("        %s(id, data, SIGNALS, SIGNAL_COUNT, &usbDevice);"
                            % message.handler)
                for signal in (s for s in message.signals if not s.ignore):
                    if signal.handler:
                        print ("        extern %s("
                            "CanSignal*, CanSignal*, int, float, bool*);" %
                            signal.handler)
                        print ("        translateCanSignal(&usbDevice, &SIGNALS[%d], "
                            "data, &%s, SIGNALS, SIGNAL_COUNT); // %s" % (
                                signal.array_index,
                                signal.handler.split()[1],
                                signal.name))
                    else:
                        print ("        translateCanSignal(&usbDevice, &SIGNALS[%d], "
                                "data, SIGNALS, SIGNAL_COUNT); // %s" % (
                                    signal.array_index, signal.name))
                print "        break;"
        print "    }"
        print "}\n"

        # Create a set of filters.
        self.print_filters()

    def print_filters(self):
        # TODO These cast a really wide net and should also be defined at the
        # top level of the JSON
        can1_masks = [(0, 0x7ff),
                (1, 0x7ff),
                (2, 0x7ff),
                (3, 0x7ff)]
        can2_masks = list(can1_masks)

        # These arrays can't be initialized when we create the variables or else
        # they end up in the .data portion of the compiled program, and it
        # becomes too big for the microcontroller. Initializing them at runtime
        # gets around that problem.
        print "CanFilterMask FILTER_MASKS[%d];" % (
                max(len(can1_masks), len(can2_masks)))

        message_count = sum((len(messages) for messages in self.buses.values()))
        print "CanFilter FILTERS[%d];" % message_count

        # TODO when the masks are defined in JSON we can do this more
        # dynamically like the filters
        print
        print "CanFilterMask* initializeFilterMasks(uint32_t address, int* count) {"
        print "Serial.println(\"Initializing filter arrays...\");"

        print "    if(address == CAN_1_ADDRESS) {"
        print "        *count = %d;" % len(can1_masks)
        print "        FILTER_MASKS = {"
        for i, mask in enumerate(can1_masks):
            print "            {%d, 0x%x}," % mask
        print "        };"
        print "    } else if(address == CAN_2_ADDRESS) {"
        print "        *count = %d;" % len(can2_masks)
        print "        FILTER_MASKS = {"
        for i, mask in enumerate(can2_masks):
            print "            {%d, 0x%x}," % mask
        print "        };"
        print "    }"
        print "    return FILTER_MASKS;"
        print "}"

        print
        print "CanFilter* initializeFilters(uint32_t address, int* count) {"
        print "Serial.println(\"Initializing filters...\");"

        print "    switch(address) {"
        for bus_address, messages in self.buses.iteritems():
            print "    case %s:" % bus_address
            print "        *count = %d;" % len(messages)
            print "        FILTERS = {"
            for i, message in enumerate(messages):
                # TODO be super smart and figure out good mask values dynamically
                print "            {%d, 0x%x, %d, %d}," % (i, message.id, 1, 0)
            print "        };"
            print "        break;"
        print "    }"
        print "    return FILTERS;"
        print "}"


class JsonParser(Parser):
    def __init__(self, filenames):
        super(JsonParser, self).__init__()
        if not hasattr(filenames, "__iter__"):
            filenames = [filenames]
        else:
            filenames = itertools.chain(*filenames)
        self.json_files = filenames

    # The JSON parser accepts the format specified in the README.
    def parse(self):
        import json
        merged_dict = {}
        for filename in itertools.chain(self.json_files):
            with open(filename) as json_file:
                data = json.load(json_file)
                merged_dict = merge(merged_dict, data)

        for bus_address, bus_data in merged_dict.iteritems():
            for message_name, message_data in bus_data['messages'].iteritems():
                self.signal_count += len(message_data['signals'])
                message = Message(message_data.get('id', None), message_name,
                        message_data.get('handler', None))
                for signal_name, signal in message_data['signals'].iteritems():
                    states = [SignalState(value, name)
                            for name, value in signal.get('states',
                                {}).iteritems()]
                    # TODO we're keeping the numerical ID here even though
                    # we're not using it now because it will make switching
                    # to it in the future easier
                    message.signals.append(
                            Signal(signal.get('id', 0),
                            signal_name,
                            signal['generic_name'],
                            signal.get('bit_position', None),
                            signal.get('bit_size', None),
                            signal.get('factor', None),
                            signal.get('offset', None),
                            signal.get('min_value', None),
                            signal.get('max_value', None),
                            signal.get('value_handler', None),
                            signal.get('ignore', False),
                            states))
                self.buses[bus_address].append(message)

def main():
    arguments = parse_options()

    parser = JsonParser(arguments.json_files)

    parser.parse()
    parser.print_source()

if __name__ == "__main__":
    sys.exit(main())

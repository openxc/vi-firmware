#!/usr/bin/env python

from collections import defaultdict
import sys
import struct
import argparse
import operator

# XXXX UGGGGGG Hack because this code is stupid.
# XXXX Should really just parse XML into some intermediate structure and then
# XXXX generate hex, code, etc. all from that format.
id_mapping = {}
# XXXX End ugly hack.

def parse_options():
    parser = argparse.ArgumentParser(description="Generate C source code from "
            "CAN signal descriptions in JSON or hex")
    hex_arg = parser.add_argument("-x", "--hex",
            action="store",
            dest="hex_file",
            metavar="FILE",
            help="generate source from this hex file")
    parser.add_argument("-j", "--json",
            action="append",
        type=str,
        nargs='*',
            dest="json_files",
            metavar="FILE",
            help="generate source from this JSON file")
    parser.add_argument('-p', '--priority',
            action='append',
            nargs='*',
            type=int,
            help='Ordered list of prioritized messages.')

    arguments = parser.parse_args()

    # Flatten the priority list.
    arguments.priority = arguments.priority or []
    if len(arguments.priority) > 0:
        arguments.priority = [item for sublist in arguments.priority
                for item in sublist]

    if arguments.hex_file and arguments.json_files:
        raise argparse.ArgumentError(hex_arg,
                "Can't specify both a hex and JSON file -- pick one!")
    if not arguments.hex_file and not arguments.json_files:
        raise argparse.ArgumentError(hex_arg,
                "Must specify either a hex file or JSON file.")

    return arguments

class Signal(object):
    def __init__(self, id, name, generic_name, position, length, factor=1,
            offset=0, value_handler=None, states=None):
        self.id = id
        self.name = name
        self.generic_name = generic_name
        self.position = position
        self.length = length
        self.factor = factor
        self.offset = offset
        self.value_handler = value_handler
        self.array_index = 0
        self.states = states or []

    def __str__(self):
        return "{%d, \"%s\", %s, %d, %d, %f, SIGNAL_STATES[%d], %d} // %s" % (
                self.id, self.generic_name, self.position, self.length,
                self.factor, self.offset, self.id, len(self.states), self.name)


class SignalState(object):
    def __init__(self, value, name):
        self.value = value
        self.name = name

    def __str__(self):
        return "{%d, \"%s\"}" % (self.value, self.name)


class Parser(object):
    def __init__(self, priority):
        self.messages = defaultdict(list)
        self.message_ids = []
        self.signal_count = 0
        self.priority = priority

    def parse(self):
        raise NotImplementedError

    def print_header(self):
        print "#include \"canutil.h\"\n"
        print "void decodeCanMessage(int id, uint8_t* data) {"

    def print_source(self):
        self.print_header()
        # TODO need to handle signals with more than 10 states
        print "    CanSignalState SIGNAL_STATES[%d][%d] = {" % (
                self.signal_count, 10)

        for signals in self.messages.values():
            for signal in signals:
                print "        {",
                for state in signal.states:
                    print "%s," % state,
                print "},"
        print "    };"

        print "    CanSignal SIGNALS[%d] = {" % self.signal_count

        i = 1
        for signals in self.messages.values():
            for signal in signals:
                signal.array_index = i - 1
                print "        %s," % signal,
                i += 1
        print "    };"

        print "    switch (id) {"
        for message_id, signals in self.messages.iteritems():
            print "    case 0x%x:" % message_id
            for signal in signals:
                if signal.value_handler:
                    print ("        extern float %s("
                        "CanSignal*, CanSignal*, float);" %
                        signal.value_handler)
                    print ("        translateCanSignal(&SIGNALS[%d], "
                        "data, &%s, SIGNALS);" % (
                            signal.array_index, signal.value_handler))
                else:
                    print "        translateCanSignal(&SIGNALS[%d], data, SIGNALS);" % (
                        signal.array_index)
            print "        break;"
        print "    }"
        print "}\n"

        # Create a set of filters.
        self.print_filters()

    def print_filters(self):
        priority_ids = [id_mapping[p] for p in self.priority if p in id_mapping]
        remaining_ids = [i for i in self.message_ids if i not in priority_ids]
        all_ids = priority_ids + remaining_ids

        # TODO these aren't correct
        masks = [(0, 0x7ff),
                (1, 0x7ff),
                (2, 0x7ff),
                (3, 0x7ff)]

        # These arrays can't be initialized when we create the variables or else
        # they end up in the .data portion of the compiled program, and it
        # becomes too big for the microcontroller. Initializing them at runtime
        # gets around that problem.
        print "int FILTER_MASK_COUNT = %d;" % len(masks)
        print "CanFilterMask FILTER_MASKS[%d];" % len(masks)
        print "int FILTER_COUNT = %d;" % len(all_ids)
        print "CanFilter FILTERS[%d];" % len(all_ids)

        print
        print "CanFilterMask* initializeFilterMasks() {"
        print "Serial.println(\"Initializing filter arrays...\");"

        print "    FILTER_MASKS = {"
        for i, mask in enumerate(masks):
            print "        {%d, 0x%x}," % mask
        print "    };"
        print "    return FILTER_MASKS;"
        print "}"

        print
        print "CanFilter* initializeFilters() {"
        print "Serial.println(\"Initializing filters...\");"

        print "    FILTERS = {"
        for i, filter in enumerate(all_ids):
            # TODO what is the relationship between mask and filter? mask is a
            # big brush that catches a bunch of things, then filter does the
            # fine grained?
            print "        {%d, 0x%x, %d, %d}," % (i, all_ids[0], 1, 0)
        print "    };"
        print "    return FILTERS;"
        print "}"


class HexParser(Parser):
    def __init__(self, filename, priority):
        super(HexParser, self).__init__(priority)
        import intelhex
        self.mem = intelhex.IntelHex(filename)

    def parse(self):
        hex_offset = 1
        while hex_offset < len(self.mem):
            (message_id, num) = struct.unpack('<HB',
                    self.mem.gets(hex_offset, 3))
            self.message_ids.append(message_id)
            hex_offset += 3
            for i in range(num):
                hex_offset, signal = self.parse_signal(message_id, hex_offset)
                self.signal_count += 1
                self.messages[message_id].append(signal)

    def parse_signal(self, message_id, hex_offset):
        (signal_id, t_pos, length) = struct.unpack('<BBB',
                self.mem.gets(hex_offset, 3))
        hex_offset += 3
        position = t_pos & ~(1 << 7)
        transform = (t_pos & 1 << 7) != 0
        if transform:
            (offset, factor) = struct.unpack('<ff',
                    self.mem.gets(hex_offset, 8))
            hex_offset += 8
        else:
            (offset, factor) = (0.0, 1.0)

        id_mapping[signal_id] = message_id
        return hex_offset, Signal(signal_id, "", "", position, length, factor,
                offset)

class JsonParser(Parser):
    def __init__(self, filenames, priority):
        super(JsonParser, self).__init__(priority)
        self.jsonFiles = filenames

    # The JSON parser accepts the format specified in the README.
    def parse(self):
        import json
        for filename in self.jsonFiles:
            with open(filename[0]) as jsonFile:
                self.data = json.load(jsonFile)
                for message in self.data['messages'].values():
                    self.message_ids.append(message['id'])
                    self.signal_count += len(message['signals'])
                    for signal in message['signals']:
                        states = [SignalState(value, name)
                                for name, value in signal.get('states',
                                    {}).iteritems()]
                        # TODO we're keeping the numerical ID here even though
                        # we're not using it now because it will make switching
                        # to it in the future easier
                        self.messages[message['id']].append(
                                Signal(signal.get('id', 0),
                                signal['name'],
                                signal['generic_name'],
                                signal['bit_position'],
                                signal['bit_size'],
                                signal.get('factor', 1),
                                signal.get('offset', 0),
                                signal.get('value_handler', None),
                                states))

def main():
    arguments = parse_options()

    if arguments.hex_file:
        parser = HexParser(arguments.hex_file, arguments.priority)
    else:
        parser = JsonParser(arguments.json_files, arguments.priority)

    parser.parse()
    parser.print_source()

if __name__ == "__main__":
    sys.exit(main())

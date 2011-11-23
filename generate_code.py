#!/usr/bin/env python

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
        result =  "{%d, \"%s\", %s, %d, %f, %f" % (
                self.id, self.generic_name, self.position, self.length,
                self.factor, self.offset)
        if len(self.states) > 0:
            result += ", SIGNAL_STATES[%d], %d" % (self.id, len(self.states))
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
        self.messages = defaultdict(list)
        self.message_ids = {}
        self.id_mapping = {}
        self.signal_count = 0

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
                if len(signal.states) > 0:
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
                print "        %s" % signal
                i += 1
        print "    };"

        print "    switch (id) {"
        for message_id, signals in self.messages.iteritems():
            print "    case 0x%x:" % message_id
            for signal in signals:
                if signal.value_handler:
                    print ("        extern %s("
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
        # TODO These cast a really wide net and should also be defined at the
        # top level of the JSON
        high_speed_masks = [(0, 0x7ff),
                (1, 0x7ff),
                (2, 0x7ff),
                (3, 0x7ff)]
        infotainment_masks = list(high_speed_masks)

        # These arrays can't be initialized when we create the variables or else
        # they end up in the .data portion of the compiled program, and it
        # becomes too big for the microcontroller. Initializing them at runtime
        # gets around that problem.
        print "CanFilterMask FILTER_MASKS[%d];" % (
                max(len(high_speed_masks), len(infotainment_masks)))

        message_count = sum((len(message_ids) for message_ids in
                self.message_ids.values()))
        print "CanFilter FILTERS[%d];" % message_count

        # TODO when the masks are defined in JSON we can do this more
        # dynamically like the filters
        print
        print "CanFilterMask* initializeFilterMasks(uint32_t address, int* count) {"
        print "Serial.println(\"Initializing filter arrays...\");"

        print "    if(address == NODE_1_CAN_1_ADDRESS) {"
        print "        *count = %d" % len(high_speed_masks)
        print "        FILTER_MASKS = {"
        for i, mask in enumerate(high_speed_masks):
            print "            {%d, 0x%x}," % mask
        print "        };"
        print "    } else if(address == NODE_1_CAN_2_ADDRESS) {"
        print "        *count = %d" % len(infotainment_masks)
        print "        FILTER_MASKS = {"
        for i, mask in enumerate(infotainment_masks):
            print "            {%d, 0x%x}," % mask
        print "        };"
        print "    }"
        print "    return FILTER_MASKS;"
        print "}"

        print
        print "CanFilter* initializeFilters(uint32_t address, int* count) {"
        print "Serial.println(\"Initializing filters...\");"

        print "    switch(address) {"
        for bus_address, message_ids in self.message_ids.iteritems():
            print "    case %s:" % bus_address
            print "        *count = %d" % len(message_ids)
            print "        FILTERS = {"
            for i, can_filter in enumerate(message_ids):
                # TODO be super smart and figure out good mask values dynamically
                print "            {%d, 0x%x, %d, %d}," % (i, can_filter, 1, 0)
            print "        };"
        print "    }"
        print "    return FILTERS;"
        print "}"


class JsonParser(Parser):
    def __init__(self, filenames):
        super(JsonParser, self).__init__()
        self.jsonFiles = filenames

    # The JSON parser accepts the format specified in the README.
    def parse(self):
        import json
        for filename in self.jsonFiles:
            with open(filename[0]) as jsonFile:
                data = json.load(jsonFile)
                bus = data['bus_address']
                self.message_ids[bus] = []
                for message in data['messages'].values():
                    self.message_ids[bus].append(message['id'])
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

    parser = JsonParser(arguments.json_files)

    parser.parse()
    parser.print_source()

if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python

from __future__ import print_function
import itertools
import operator
from collections import defaultdict
import sys
import argparse
import os

from common import warning, fatal_error, Signal, SignalState, Message, \
        Command, merge, valid_buses, all_messages, find_file


MAX_SIGNAL_STATES = 12
DEFAULT_SEARCH_PATH = "."

base_path = os.path.dirname(sys.argv[0])


def parse_options():
    parser = argparse.ArgumentParser(description="Generate C++ source code "
            "from CAN signal descriptions in JSON")
    json_files = parser.add_argument("-j", "--json",
            action="append",
            type=str,
            nargs='*',
            dest="json_files",
            metavar="FILE",
            help="generate source from this JSON file")
    parser.add_argument("-s", "--search-paths",
            type=str,
            nargs='+',
            dest="search_paths",
            default=None,
            metavar="PATH",
            help="add directories to the search path when using relative paths")
    parser.add_argument("-m", "--message-set",
            action="store", type=str, dest="message_set", metavar="MESSAGE_SET",
            default=None, help="override name of vehicle or platform")

    arguments = parser.parse_args()

    if not arguments.json_files:
        raise argparse.ArgumentError(json_files,
                "Must specify at least one JSON file.")

    return arguments


class Parser(object):
    def __init__(self, name=None):
        self.name = name
        self.buses = defaultdict(dict)
        self.signal_count = 0
        self.message_count = 0
        self.command_count = 0
        self.initializers = []
        self.loopers = []
        self.commands = []

    def parse(self):
        raise NotImplementedError

    def build_header(self):
        result = ""
        with open("%s/signals.cpp.header" % base_path) as header:
            result += header.read()

        if getattr(self, 'uses_custom_handlers', None):
            '\n'.join([result,
                "#include \"handlers.h\"",
                "using namespace openxc::signals::handlers;"])
        return result

    def validate_messages(self):
        valid = True
        for message in all_messages(self.buses):
            if message.handler is not None:
                self.uses_custom_handlers = True
            for signal in message.signals:
                valid = valid and signal.validate()
                if signal.handler is not None:
                    self.uses_custom_handlers = True
        return valid

    def validate_name(self):
        if self.name is None:
            warning("missing message set (%s)" % self.name)
            return False
        return True

    def _build_bus_struct(self, bus_address, bus, bus_number):
        result = """    {{ {bus_speed}, {bus_address}, can{bus_number},
        #ifdef __PIC32__
        handleCan{bus_number}Interrupt,
        #endif // __PIC32__
    }},"""
        return result.format(bus_speed=bus['speed'], bus_address=bus_address,
                bus_number=bus_number)

    def build_source(self):
        if not self.validate_messages() or not self.validate_name():
            fatal_error("unable to generate code")
        lines = [self.build_header()]

        lines.append("const int CAN_BUS_COUNT = %d;" % len(
                list(valid_buses(self.buses))))
        lines.append("CanBus CAN_BUSES[CAN_BUS_COUNT] = {")
        for bus_number, (bus_address, bus) in enumerate(
                valid_buses(self.buses)):
            lines.append(self._build_bus_struct(bus_address, bus, bus_number +
                1))
            lines.append("")

        lines.append("};")
        lines.append("")

        lines.append("const int MESSAGE_COUNT = %d;" % self.message_count)
        lines.append("CanMessage CAN_MESSAGES[MESSAGE_COUNT] = {")

        for i, message in enumerate(all_messages(self.buses)):
            message.array_index = i
            lines.append("    %s" % message)
        lines.append("};")
        lines.append("")

        lines.append("const int SIGNAL_COUNT = %d;" % self.signal_count)
        lines.append(("CanSignalState SIGNAL_STATES[SIGNAL_COUNT][%d] = {"
                % MAX_SIGNAL_STATES))

        states_index = 0
        for message in all_messages(self.buses):
            for signal in message.signals:
                if len(signal.states) > 0:
                    if states_index >= MAX_SIGNAL_STATES:
                        warning("Ignoring anything beyond %d states for %s" %
                                (MAX_SIGNAL_STATES, signal.generic_name))
                        break

                    lines.append("    {", end=' ')
                    for state in signal.states:
                        lines.append("%s," % state, end=' ')
                    lines.append("},")
                    signal.states_index = states_index
                    states_index += 1
        lines.append("};")
        lines.append("")

        lines.append("CanSignal SIGNALS[SIGNAL_COUNT] = {")

        i = 1
        for message in all_messages(self.buses):
            message.signals = sorted(message.signals,
                    key=operator.attrgetter('generic_name'))
            for signal in message.signals:
                signal.array_index = i - 1
                lines.append("    %s" % signal)
                i += 1
        lines.append("};")
        lines.append("")

        lines.append("void openxc::signals::initializeSignals() {")
        for initializer in self.initializers:
            lines.append("    %s();" % initializer)
        lines.append("}")
        lines.append("")

        lines.append("void openxc::signals::loop() {")
        for looper in self.loopers:
            lines.append("    %s();" % looper)
        lines.append("}")
        lines.append("")

        lines.append("const int COMMAND_COUNT = %d;" % self.command_count)
        lines.append("CanCommand COMMANDS[COMMAND_COUNT] = {")

        for command in self.commands:
            lines.append("    ", command)

        lines.append("};")
        lines.append("")

        with open("%s/signals.cpp.middle" % base_path) as middle:
            lines.append(middle.read())

        lines.append("const char* openxc::signals::getMessageSet() {")
        lines.append("    return \"%s\";" % self.name)
        lines.append("}")
        lines.append("")

        lines.append("void openxc::signals::decodeCanMessage(Pipeline* pipeline, "
                "CanBus* bus, int id, uint64_t data) {")
        lines.append("    switch(bus->address) {")
        for bus_address, bus in valid_buses(self.buses):
            lines.append("    case %s:" % bus_address)
            lines.append("        switch (id) {")
            for message in bus['messages']:
                lines.append("        case 0x%x: // %s" % (message.id, message.name))
                if message.handler is not None:
                    lines.append(("            %s(id, data, SIGNALS, " %
                        message.handler + "SIGNAL_COUNT, pipeline);"))
                for signal in (s for s in message.signals):
                    if signal.handler:
                        lines.append(("            can::read::translateSignal("
                                "pipeline, "
                                "&SIGNALS[%d], data, " % signal.array_index +
                                "&%s, SIGNALS, SIGNAL_COUNT); // %s" % (
                                signal.handler, signal.name)))
                    else:
                        lines.append(("            can::read::translateSignal("
                                "pipeline, "
                                "&SIGNALS[%d], " % signal.array_index +
                                "data, SIGNALS, SIGNAL_COUNT); // %s"
                                    % signal.name))
                lines.append("            break;")
            lines.append("        }")
            lines.append("        break;")
        lines.append("    }")

        if self._message_count() == 0:
            lines.append("    openxc::can::read::passthroughMessage(pipeline, id, "
                    "data);")

        lines.append("}")
        lines.append("")

        # Create a set of filters.
        lines.append(self.build_filters())
        lines.append("")
        lines.append("#endif // CAN_EMULATOR")

        return '\n'.join(lines)

    def _message_count(self):
        return len(list(all_messages(self.buses)))

    def build_filters(self):
        # These arrays can't be initialized when we create the variables or else
        # they end up in the .data portion of the compiled program, and it
        # becomes too big for the microcontroller. Initializing them at runtime
        # gets around that problem.
        lines = []
        lines.append("CanFilter FILTERS[%d];" % self._message_count())

        lines.append("")
        lines.append("CanFilter* openxc::signals::initializeFilters(uint64_t address, "
                "int* count) {")
        lines.append("    switch(address) {")
        for bus_address, bus in valid_buses(self.buses):
            lines.append("    case %s:" % bus_address)
            lines.append("        *count = %d;" % len(bus['messages']))
            for i, message in enumerate(bus['messages']):
                lines.append("        FILTERS[%d] = {%d, 0x%x, %d};" % (
                        i, i, message.id, 1))
            lines.append("        break;")
        lines.append("    }")
        lines.append("    return FILTERS;")
        lines.append("}")
        return '\n'.join(lines)


class JsonParser(Parser):
    def __init__(self, search_paths, filenames, name=None):
        super(JsonParser, self).__init__(name)

        if not isinstance(filenames, list):
            filenames = [filenames]
        else:
            filenames = itertools.chain(*filenames)
        self.json_files = filenames

        self.search_paths = search_paths or []
        self.search_paths.append(DEFAULT_SEARCH_PATH)

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

        for parent_filename in merged_dict.get("parents", []):
            with open(find_file(parent_filename, self.search_paths)
                    ) as json_file:
                try:
                    parent_data = json.load(json_file)
                except ValueError as e:
                    fatal_error("%s does not contain valid JSON: \n%s\n" %
                            (parent_filename, e))
                # Merge merged_dict *into* parents, so we keep any overrides
                merged_dict = merge(parent_data, merged_dict)
                break

        self.name = self.name or merged_dict.get("name", "generic")
        self.initializers = merged_dict.get("initializers", [])
        self.loopers = merged_dict.get("loopers", [])

        self.buses = merged_dict.get("buses", {})
        for bus_name, bus in self.buses.items():
            if bus.get('speed', None) is None:
                fatal_error("Bus %s is missing the 'speed' attribute" %
                        bus_name)
            bus['messages'] = []

        for command in merged_dict.get('commands', []):
            self.command_count += 1
            command = Command(command['name'], command.get('handler', None))
            self.commands.append(command)

        for mapping in merged_dict.get("mappings", []):
            if 'mapping' not in mapping:
                fatal_error("Mapping is missing the mapping file path")

            with open(find_file(mapping['mapping'], self.search_paths)
                    ) as mapping_file:
                mapping_data = json.load(mapping_file)
                messages = mapping_data.get('messages', None)
                if messages is None:
                    fatal_error("Mapping file '%s' is missing a 'messages' field" % mapping['mapping'])

                if 'database' in mapping:
                    from xml_to_json import merge_database_into_mapping
                    messages = merge_database_into_mapping(
                            find_file(mapping['database'], self.search_paths),
                            messages)['messages']

                self.load_messages(messages, mapping['bus'])

        self.load_messages(merged_dict.get('messages', {}))

    def load_messages(self, messages, default_bus=None):
        for message_id, message_data in messages.items():
            self.signal_count += len(message_data['signals'])
            self.message_count += 1
            message = Message(self.buses,
                    message_data.get('bus', None) or default_bus,
                    message_id,
                    message_data.get('name', None),
                    message_data.get('handler', None))
            for signal_name, signal in message_data['signals'].items():
                states = []
                for name, raw_matches in signal.get('states', {}).items():
                    for raw_match in raw_matches:
                        states.append(SignalState(raw_match, name))
                signal.pop('states', None)
                message.signals.append(Signal(
                        self.buses,
                        message,
                        signal_name,
                        states=states,
                        **signal))
            self.buses[message.bus_name]['messages'].append(message)


def main():
    arguments = parse_options()

    parser = JsonParser(arguments.search_paths, arguments.json_files,
            arguments.message_set)

    parser.parse()
    print(parser.build_source())

if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python

from __future__ import print_function
from collections import defaultdict
import sys
import argparse
import operator

from coder import CodeGenerator
from xml_to_json import merge_database_into_mapping
from common import warning, fatal_error, Signal, SignalState, Message, \
        Command, merge, find_file, load_json_from_search_path, \
        VALID_BUS_ADDRESSES

DEFAULT_SEARCH_PATH = "."


def parse_options():
    parser = argparse.ArgumentParser(description="Generate C++ source code "
            "from CAN signal descriptions in JSON")

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-m", "--message-sets",
            type=str,
            nargs='+',
            dest="message_sets",
            metavar="MESSAGE_SET",
            help="generate source from these JSON-formatted message "
                    "set definitions")
    group.add_argument("--super-set",
            type=str,
            dest="super_set",
            metavar="SUPERSET",
            help="generate source with multiple active message sets, defined in"
                    " this JSON-formatted superset")
    parser.add_argument("-s", "--search-paths",
            type=str,
            nargs='+',
            dest="search_paths",
            metavar="PATH",
            help="add directories to the search path when using relative paths")

    arguments = parser.parse_args()

    return arguments


class MessageSet(object):
    def __init__(self, name):
        self.name = name
        self.buses = defaultdict(dict)
        self.signal_count = 0
        self.command_count = 0
        self.initializers = []
        self.loopers = []
        self.commands = []
        self.extra_sources = []

    def valid_buses(self):
        for bus_name, bus in sorted(self.buses.items(), key=operator.itemgetter(0)):
            if bus['controller'] in VALID_BUS_ADDRESSES:
                yield bus['controller'], bus

    def all_messages(self):
        for _, bus in self.valid_buses():
            for message in bus['messages']:
                yield message

    def all_signals(self):
        for message in self.all_messages():
            for signal in message.signals:
                yield signal

    def validate_messages(self):
        valid = True
        for message in self.all_messages():
            for signal in message.signals:
                valid = valid and signal.validate()
        return valid

    def validate_name(self):
        if self.name is None:
            warning("missing message set (%s)" % self.name)
            return False
        return True

    def _message_count(self):
        return len(list(self.all_messages()))


class JsonMessageSet(MessageSet):
    @classmethod
    def parse(cls, filename, search_paths=None):
        search_paths = search_paths or []

        data = load_json_from_search_path(filename, search_paths)

        while len(data.get('parents', [])) > 0:
            for parent_filename in data.get('parents', []):
                parent_data = load_json_from_search_path(parent_filename,
                        search_paths)
                # Merge data *into* parents, so we keep any overrides
                data = merge(parent_data, data)
                data['parents'].remove(parent_filename)

        message_set = cls(data.get('name', 'generic'))
        message_set.initializers = data.get('initializers', [])
        message_set.loopers = data.get('loopers', [])
        message_set.buses = cls._parse_buses(data)
        message_set.extra_sources = data.get('extra_sources', [])
        message_set.commands = cls._parse_commands(data)
        message_set._parse_mappings(data, search_paths)
        message_set._parse_messages(data.get('messages', {}))

        return message_set

    @classmethod
    def _parse_commands(cls, data):
        return [Command(command['name'], command.get('handler', None))
                for command in data.get('commands', [])]

    @classmethod
    def _parse_buses(cls, data):
        buses = data.get('buses', {})
        for bus_name, bus in buses.items():
            if bus.get('speed', None) is None:
                fatal_error("Bus %s is missing the 'speed' attribute" %
                        bus_name)
            bus['messages'] = []
        return buses

    def _parse_mappings(self, data, search_paths):
        all_messages = {}
        for mapping in data.get('mappings', []):
            if 'mapping' not in mapping:
                fatal_error("Mapping is missing the mapping file path")

            bus_name = mapping.get('bus', None)
            if bus_name is None:
                warning("No default bus associated with '%s' mapping" %
                        mapping['mapping'])
            elif bus_name not in self.buses:
                fatal_error("Bus '%s' (from mapping %s) is not defined" %
                        (bus_name, mapping['mapping']))

            mapping_data = load_json_from_search_path(mapping['mapping'],
                    search_paths)
            messages = mapping_data.get('messages', None)
            if messages is None:
                fatal_error("Mapping file '%s' is missing a 'messages' field"
                        % mapping['mapping'])

            if 'database' in mapping:
                messages = merge(merge_database_into_mapping(
                            find_file(mapping['database'], search_paths),
                            messages)['messages'],
                        messages)

            for message in messages.values():
                if 'bus' not in message:
                    message['bus'] = bus_name

            all_messages = merge(all_messages, messages)
        self._parse_messages(all_messages)

    def _parse_messages(self, messages, default_bus=None):
        for message_id, message_data in messages.items():
            self.signal_count += len(message_data['signals'])
            message = Message(self.buses,
                    message_data.get('bus', None),
                    message_id,
                    message_data.get('name', None),
                    message_data.get('handler', None))
            if message.bus_name is None:
                fatal_error("No default or explicit bus for message %s" %
                        message_id)
            for signal_name, signal in message_data['signals'].items():
                states = []
                for name, raw_matches in signal.get('states', {}).items():
                    for raw_match in raw_matches:
                        states.append(SignalState(raw_match, name))
                signal.pop('states', None)
                message.signals.append(Signal(
                        self,
                        message,
                        signal_name,
                        states=states,
                        **signal))
            if message.bus_name not in self.buses:
                fatal_error("Bus '%s' (from message 0x%x) is not defined" %
                        (message.bus_name, message.id))
            self.buses[message.bus_name]['messages'].append(message)


def main():
    arguments = parse_options()

    search_paths = arguments.search_paths or []
    search_paths.append(DEFAULT_SEARCH_PATH)

    message_sets = arguments.message_sets or []
    if arguments.super_set is not None:
        super_set_data = load_json_from_search_path(arguments.super_set,
                arguments.search_paths)
        super_set_message_sets = super_set_data.get('message_sets', [])
        if len(super_set_message_sets) == 0:
            warning("Superset '%s' has no message sets" %
                    super_set_data.get('name', 'unknown'))
        message_sets.extend(super_set_message_sets)

    generator = CodeGenerator(search_paths)
    for filename in message_sets:
        message_set = JsonMessageSet.parse(filename, search_paths=search_paths)
        if not message_set.validate_messages() or not message_set.validate_name():
            fatal_error("unable to generate code")
        generator.message_sets.append(message_set)

    print(generator.build_source())

if __name__ == "__main__":
    sys.exit(main())

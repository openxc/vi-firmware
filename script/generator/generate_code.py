#!/usr/bin/env python

from __future__ import print_function
from collections import defaultdict
import sys
import argparse
import json

from coder import CodeGenerator
from xml_to_json import merge_database_into_mapping
from common import warning, fatal_error, Signal, SignalState, Message, \
        Command, merge, all_messages, find_file, load_json_from_search_path

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
    def __init__(self, name=None):
        self.buses = defaultdict(dict)
        self.signal_count = 0
        self.command_count = 0
        self.initializers = []
        self.loopers = []
        self.commands = []

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

    def _message_count(self):
        return len(list(all_messages(self.buses)))


class JsonMessageSet(MessageSet):
    @classmethod
    def parse(cls, filename, search_paths=None):
        search_paths = search_paths or []

        data = {}
        with open(find_file(filename, search_paths)) as json_file:
            try:
                data = json.load(json_file)
            except ValueError as e:
                fatal_error("%s does not contain valid JSON: \n%s\n" %
                        (filename, e))

        for parent_filename in data.get("parents", []):
            with open(find_file(parent_filename, search_paths)
                    ) as json_file:
                try:
                    parent_data = json.load(json_file)
                except ValueError as e:
                    fatal_error("%s does not contain valid JSON: \n%s\n" %
                            (parent_filename, e))
                # Merge data *into* parents, so we keep any overrides
                data = merge(parent_data, data)

        message_set = cls(data.get("name", "generic"))
        message_set.initializers = data.get("initializers", [])
        message_set.loopers = data.get("loopers", [])
        message_set.buses = cls._parse_buses(data)
        message_set.commands = cls._parse_commands(data)
        message_set._parse_mappings(data, search_paths)
        message_set._parse_messages(data.get('messages', {}))

        return message_set

    @classmethod
    def _parse_commands(cls, data):
        return (Command(command['name'], command.get('handler', None))
                for command in data.get('commands', []))

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
        for mapping in data.get("mappings", []):
            if 'mapping' not in mapping:
                fatal_error("Mapping is missing the mapping file path")

            with open(find_file(mapping['mapping'], search_paths)
                    ) as mapping_file:
                mapping_data = json.load(mapping_file)
                messages = mapping_data.get('messages', None)
                if messages is None:
                    fatal_error("Mapping file '%s' is missing a 'messages' field"
                            % mapping['mapping'])

                if 'database' in mapping:
                    messages = merge_database_into_mapping(
                            find_file(mapping['database'], search_paths),
                            messages)['messages']

                self._parse_messages(messages, mapping['bus'])

    def _parse_messages(self, messages, default_bus=None):
        for message_id, message_data in messages.items():
            self.signal_count += len(message_data['signals'])
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

    search_paths = arguments.search_paths
    search_paths.append(DEFAULT_SEARCH_PATH)

    message_sets = []
    message_sets.extend(arguments.message_sets)
    if arguments.super_set is not None:
        super_set_data = load_json_from_search_path(arguments.super_set,
                arguments.search_paths)
        message_sets.extend(super_set_data.get('message_sets', []))

    generator = CodeGenerator()
    for filename in message_sets:
        generator.message_sets += JsonMessageSet.parse(filename,
                search_paths=search_paths)

    print(generator.build_source())

if __name__ == "__main__":
    sys.exit(main())

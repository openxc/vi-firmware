#!/usr/bin/env python

import sys
import argparse
import json
from xml.etree.ElementTree import parse

from generate_code import JsonParser, Signal


class Network(object):
    """Represents all the messages on a single bus."""

    def __init__(self, tree, address, all_messages=None):
        self.address = address
        self.messages = {}

        for node in tree.getroot().findall("Node"):
            self._parse_node(node, all_messages)

    def to_dict(self):
        return {self.address: {"messages": dict(("0x%x" % message.id,
                    message.to_dict())
                for message in list(self.messages.values())
                if len(message.signals) > 0)}}

    def _parse_node(self, node, all_messages):
        # Looks like RxMessage elements are redundant.
        for message_node in node.findall("TxMessage"):
            message = Message(message_node, all_messages)
            self.messages[message.id] = message


class Message(object):
    """Contains a single CAN message."""

    def __init__(self, node, all_messages=None):
        self.signals = []

        # XXX Is the number of bytes in DLC?
        self.name = node.find("Name").text
        self.id = int(node.find("ID").text, 0)

        for message in all_messages:
            if message.id == self.id:
                signals = (Signal.from_xml_node(signal_node)
                        for signal_node in node.findall("Signal"))
                for signal in signals:
                    for candidate in message.signals:
                        if candidate.name == signal.name:
                            signal.generic_name = candidate.generic_name
                            self.signals.append(signal)

    def to_dict(self):
        return {"name": self.name,
                "signals": dict((signal.name, signal.to_dict())
                    for signal in self.signals)}


def parse_options(argv):
    parser = argparse.ArgumentParser(
            description="Convert Canoe XML to the OpenXC JSON format.")
    parser.add_argument("xml", help="Name of Canoe XML file")
    parser.add_argument("mapping_filename",
            help="Path to a JSON file with CAN messages mapped to OpenXC names")
    parser.add_argument("out", default="dump.json",
            help="Name out output JSON file")

    return parser.parse_args(argv)


def main(argv=None):
    arguments = parse_options(argv)

    parser = JsonParser(arguments.mapping_filename)
    parser.parse()
    message_count = sum((len(bus.get('messages', {}))
            for bus in list(parser.buses.values())))
    if len(parser.buses) > 1:
        raise RuntimeError("No more than one CAN bus can be defined")
    elif len(parser.buses) == 0 or message_count == 0:
        data = {}
    else:
        tree = parse(arguments.xml)
        bus_address, bus = list(parser.buses.items())[0]
        n = Network(tree, bus_address, bus['messages'])
        data = n.to_dict()

    with open(arguments.out, 'w') as output_file:
        json.dump(data, output_file, indent=4)
    print("Wrote results to %s" % arguments.out)

if __name__ == "__main__":
    sys.exit(main())

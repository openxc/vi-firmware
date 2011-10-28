#!/usr/bin/env python

import sys
import logging
import argparse
import struct
import json
from xml.etree.ElementTree import parse


class Network(object):
    """Represents all the messages on a single bus."""

    def __init__(self, tree, signal_map = None):
        self.messages = {}

        for node in tree.getroot().findall("Node"):
            self._parse_node(node, signal_map)

    def to_dict(self):
        return {"messages": {message.name: message.to_dict()
                for message in self.messages.values()
                if len(message.signals) > 0}}

    def _parse_node(self, node, signal_map):
        # Looks like RxMessage elements are redundant.
        for message_node in node.findall("TxMessage"):
            message = Message(message_node, signal_map)
            self.messages[message.id] = message


class Message(object):
    """Contains a single CAN message."""

    def __init__(self, node, signal_map = None):
        self.signals = {}

        # XXX Is the number of bytes in DLC?
        self.name = node.find("Name").text
        self.id = int(node.find("ID").text, 0)

        for signal_node in node.findall("Signal"):
            signal = Signal(signal_node)
            if signal.name in signal_map:
                signal.id = signal_map[signal.name]
                self.signals[signal.position] = signal

    def to_dict(self):
        return {"id": self.id,
                "signals": [signal.to_dict()
                    for signal in self.signals.values()]}


class Signal(object):
    """Contains a single CAN signal."""

    def __init__(self, node):
        self.name = node.find("Name").text
        self.position = int(node.find("Bitposition").text)
        self.size = int(node.find("Bitsize").text)
        self.factor = float(node.find("Factor").text)
        self.offset = float(node.find("Offset").text)

        # Invert the bit index to match the Excel mapping.
        self.position = self._invert_bit_index(self.position, self.size)


    def to_dict(self):
        return {"id": self.id,
                "name": self.name,
                "bit_position": self.position,
                "bit_size": self.size,
                "factor": self.factor,
                "offset": self.offset}

    def _invert_bit_index(self, i, l):
        (b, r) = divmod(i, 8)
        end = (8 * b) + (7 - r)
        return(end - l + 1)


def parse_map(mapping_filename):
    """Parses a text file that maps signal names to look up in the Canoe XML
    document with a unique numerical ID.

    The expected file format is:

        1:SteeringAngle
        2:VehYawComp_W_Actl
        6:EngAout_N_Actl
        ...

    """
    sig_map = {}

    with open(mapping_filename) as mapping_file:
        for line in mapping_file:
            try:
                signal_id, name = line.strip().split(':', 1)
                sig_map[name] = int(signal_id)
            except ValueError:
                print "Unable to parse line '%s'" % line
    return(sig_map)

def main(argv=None):
    parser = argparse.ArgumentParser(
            description="Convert Canoe XML to the OpenXC JSON format.")
    parser.add_argument("xml", default="c346_hs_mappint.txt",
            help="Name of Canoe XML file")
    parser.add_argument("map", default="c346_hs_can.xml",
            help="Name of signal to ID map")
    parser.add_argument("out", default="dump.json",
            help="Name out output JSON file")

    args = parser.parse_args(argv)

    tree = parse(args.xml)
    sig_map = parse_map(args.map)
    n = Network(tree, sig_map)

    data = n.to_dict()
    with open(args.out, 'w') as output_file:
        json.dump(data, output_file, indent=4)
    print "Wrote results to %s" % args.out

if __name__ == "__main__":
    sys.exit(main())

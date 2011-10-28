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

        elem = tree.getroot()

        for e in elem:
            if e.tag == "Node":
                self._parse_node(e, signal_map)

    def to_dict(self):
        return {"messages": {message.name: message.to_dict()
                for message in self.messages.values()}}

    def _parse_node(self, node, signal_map):
        for e in node:
            # Looks like RxMessage elements are redundant.
            if e.tag == "TxMessage":
                m = Message(e, signal_map)
                self.messages[m.id] = m


class Message(object):
    """Contains a single CAN message."""

    def __init__(self, node, signal_map = None):
        self.signals = {}

        # XXX Is the number of bytes in DLC?
        for e in node:
            if e.tag == "Name":
                self.name = e.text
                logging.debug("Message: {0}".format(self.name))
            elif e.tag == "ID":
                self.id = int(e.text, 0)
                logging.debug("Message: 0x{0:x}".format(self.id))
            elif e.tag == "Signal":
                s = Signal(e, signal_map)
                self.signals[s.position] = s

    def to_dict(self):
        return {"id": self.id,
                "signals": [signal.to_dict()
                    for signal in self.signals.values() if signal.include]}


class Signal(object):
    """Contains a single CAN signal."""

    def __init__(self, node, signal_map = None):
        for e in node:
            if e.tag == "Name":
                self.name = e.text
                logging.debug("Signal: {0}".format(self.name))
            elif e.tag == "Bitposition":
                self.position = int(e.text)
                logging.debug("Position: {0}".format(self.position))
            elif e.tag == "Bitsize":
                self.size = int(e.text)
                logging.debug("Size: {0}".format(self.size))
            elif e.tag == "Factor":
                self.factor = float(e.text)
                logging.debug("Factor: {0}".format(self.factor))
            elif e.tag == "Offset":
                self.offset = float(e.text)
                logging.debug("Offset: {0}".format(self.offset))
            elif e.tag == "Unit":
                self.unit = e.text
                logging.debug("Unit: {0}".format(self.unit))

        # Have to invert the bit index to match the Excel mapping.
        self.position = self._invert_bit_index(self.position, self.size)

        if (signal_map and self.name in signal_map):
            self.id = signal_map[self.name]
            self.include = True
        else:
            self.id = -1
            self.include = False

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

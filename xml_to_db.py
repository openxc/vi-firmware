#!/usr/bin/env python

import sys
import logging
import argparse
import struct
import intelhex
from xml.etree.ElementTree import parse

#logging.basicConfig(level=logging.DEBUG)

class Network:
    """Represents all the messages on a single bus."""

    def dump(self):
        for k in sorted(self.messages.iterkeys()):
            self.messages[k].dump()

    def pack(self, mem, offset):
        for k in sorted(self.messages.iterkeys()):
            offset = self.messages[k].pack(mem, offset)
        return(offset)
    
    def _parse_node(self, node, signal_map):
        for e in node:
            # Looks like RxMessage elements are redundant.
            if e.tag == "TxMessage":
                m = Message(e, signal_map)
                self.messages[m.id] = m
        
    def __init__(self, tree, signal_map = None):
        self.messages = {}

        elem = tree.getroot()

        for e in elem:
            if e.tag == "Node":
                self._parse_node(e, signal_map)


class Message:
    """Contains a single CAN message."""

    def dump(self):
        shown = False
        for k in sorted(self.signals.iterkeys()):
            if self.signals[k].include:
                if not shown:
                    print "message 0x{0:x}".format(self.id)
                    shown = True
                self.signals[k].dump()

    def pack(self, mem, offset):
        count = 0
        for k in sorted(self.signals.iterkeys()):
            if self.signals[k].include:
                count += 1

        if count > 0:
            s = struct.pack('<HB', self.id, count)
            mem.puts(offset, s)
            offset += len(s)
            for k in sorted(self.signals.iterkeys()):
                if self.signals[k].include:
                    offset = self.signals[k].pack(mem, offset)
        
        return(offset)
    
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

class Signal:
    """Contains a single CAN signal."""

    def dump(self):
        print ("  {position}:{size}:{transform}:{factor}:{offset}:"
               "{name}:{id}".format(
                   position = self.position, size = self.size,
                   transform = self.transform, factor = self.factor,
                   offset = self.offset, name = self.name, id = self.id))

    def pack(self, mem, offset):
        # The Arduino is little endian.
        first = struct.pack('<BBB', self.id,
                            (1 << 7 if self.transform else 0) | self.position,
                            self.size)
        mem.puts(offset, first)
        offset += len(first)
        if self.transform:
            second = struct.pack('<ff', self.offset, self.factor)
            mem.puts(offset, second)
            offset += len(second)

        return(offset)

    def _invert_bit_index(self, i, l):
        (b, r) = divmod(i, 8)
        end = (8 * b) + (7 - r)
        return(end - l + 1)

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
        
        # Transform if we're not state encoded and the factor and offset
        # are non-unit transforms.
        self.transform = (self.unit != 'SED' and
                          (self.factor != 1.0 or self.offset != 0.0))

        if (signal_map and self.name in signal_map):
            self.id = signal_map[self.name]
            self.include = True
        else:
            self.id = -1
            self.include = False


def parse_map(file):
    sig_map = {}
    
    f = open(file, 'r')
    for line in f:
        line = line.strip()
        try:
            (id, name) = line.split(':', 1)
            sig_map[name] = int(id)
        except ValueError:
            print "Unable to parse line '{0}'".format(line)

    return(sig_map)

def main(argv=None):
    parser = argparse.ArgumentParser(description="Convert Canoe XML to "
                                     "Arduino HEX database.")
    parser.add_argument('xml', default='c346_hs_mappint.txt',
                        help='Name of Canoe XML file')
    parser.add_argument('map', default='c346_hs_can.xml',
                        help='Name of signal to ID map')
    parser.add_argument('out', default='dump.hex',
                        help='Name out output HEX file')

    args = parser.parse_args(argv)
    
    sig_map = parse_map(args.map)

    tree = parse(args.xml)
    n = Network(tree, sig_map)

    n.dump()

    mem = intelhex.IntelHex()
    bytes = n.pack(mem, 1)
    mem[0] = bytes
    mem.write_hex_file(args.out)
    print 'Wrote {0} bytes to {1}'.format(len(mem), args.out)

if __name__ == "__main__":
    sys.exit(main())

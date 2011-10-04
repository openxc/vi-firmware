#!/usr/bin/env python

import sys
import struct
import argparse
import intelhex

def parse_signal(mem, offset):
    (id, t_pos, len) = struct.unpack('<BBB', mem.gets(offset, 3))
    offset += 3
    transform = (t_pos & 1 << 7) != 0
    position = t_pos & ~(1 << 7)
    if transform:
        (off, factor) = struct.unpack('<ff', mem.gets(offset, 8))
        offset += 8
    else:
        (off, factor) = (0.0, 1.0)

    print ('  {id}:{position}:{size}:{transform}:{factor}:'
           '{offset}:'.format(id = id, position = position, size = len,
                              transform = transform, factor = factor,
                              offset = off))
    return(offset)


def parse_messages(mem, offset):
    while offset < len(mem):
        (id, num) = struct.unpack('<HB', mem.gets(offset, 3))
        offset += 3
        print "message 0x{0:x}, {1} signals".format(id, num)

        for i in range(num):
            offset = parse_signal(mem, offset)

def main(argv=None):
    parser = argparse.ArgumentParser(description='Print data from hex file.')
    parser.add_argument('hex', default='dump.hex', help='Name of hex file')

    args = parser.parse_args(argv)

    mem = intelhex.IntelHex(args.hex)
    parse_messages(mem, 1)

if __name__ == "__main__":
    sys.exit(main())

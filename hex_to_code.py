#!/usr/bin/env python

import sys
import struct
import argparse
import intelhex

# XXXX UGGGGGG Hack because this code is stupid.
# XXXX Should really just parse XML into some intermediate structure and then
# XXXX generate hex, code, etc. all from that format.
id_mapping = {}
# XXXX End ugly hack.

def create_filter_code(ids, priority):
    priority_ids = [id_mapping[p] for p in priority if p in id_mapping]
    remaining_ids = [i for i in ids if i not in priority_ids]
    all_ids = priority_ids + remaining_ids

    masks = [(0, 0x7ff),
            (1, 0x7ff),
            (2, 0x7ff),
            (3, 0x7ff)]

    print "int FILTER_MASK_COUNT = %d;" % len(masks)
    print "CanFilterMask FILTER_MASKS[%d] = {" % len(masks)
    for i, mask in enumerate(masks):
        print "    {%d, 0x%x}" % mask,
        if i != len(masks) - 1:
            print ","
        else:
            print "};"
    print

    print "int FILTER_COUNT = %d;" % len(all_ids)
    print "CanFilter FILTERS[%d] = {" % len(all_ids)
    for i, filter in enumerate(all_ids):
        # TODO what is the relationship between mask and filter? mask is a big
        # brush that catches a bunch of things, then filter does the fine
        # grained?
        print "    {%d, 0x%x, %d, %d}" % (i, all_ids[0], 1, 0),
        if i != len(all_ids) - 1:
            print ","
        else:
            print "};"
    print

def parse_signal(mem, offset, message_id):
    (id, t_pos, len) = struct.unpack('<BBB', mem.gets(offset, 3))
    offset += 3
    transform = (t_pos & 1 << 7) != 0
    position = t_pos & ~(1 << 7)
    if transform:
        (off, factor) = struct.unpack('<ff', mem.gets(offset, 8))
        offset += 8
    else:
        (off, factor) = (0.0, 1.0)

    id_mapping[id] = message_id

    print ''
    print '  // Signal {0}.'.format(id)
    print '  ivalue = getBitField(data, {0}, {1});'.format(
        position, len)
    if (transform):
        print '  fvalue = (float)ivalue * {0} + {1};'.format(factor, off)

    print '  Serial.print(\'^\');'
    print '  Serial.print({0}, DEC);'.format(id)
    print '  Serial.print(\':\');'
    if (transform):
        print '  Serial.print(fvalue, 7);'
    else:
        print '  Serial.print(ivalue, DEC);'

    print '  Serial.println(\'$\');'

    return(offset)


def parse_messages(mem, offset, priority=None):
    ids = []

    while offset < len(mem):
        (id, num) = struct.unpack('<HB', mem.gets(offset, 3))
        ids.append(id)
        offset += 3

        print 'void'
        print 'decode_message_{0:x}(uint8_t* data) {{'.format(id)

        print '  unsigned long ivalue;'
        print '  float fvalue;'

        for i in range(num):
            offset = parse_signal(mem, offset, id)

        print '}'
        print ''

    # Go back and print out a message parsing block.
    print 'void'
    print 'decode_can_message(int id, uint8_t* data) {'
    print '  switch (id) {'

    for i in ids:
        print '    case {0}:'.format(i)
        print '      decode_message_{0:x}(data);'.format(i)
        print '      break;'

    print '  }'
    print '}'

    # Create a set of filters.
    create_filter_code(ids, priority)

def print_header():
    print "#include \"bitfield.h\"\n"

def main(argv=None):
    parser = argparse.ArgumentParser(description='Convert hex file to Arduino '
                                     'sketch.')
    parser.add_argument('hex', default='dump.hex', help='Name of hex file')
    parser.add_argument('-p', '--priority', action='append', nargs='*',
                        type=int, help='Ordered list of prioritized messages.')

    args = parser.parse_args(argv)

    # Flatten the priority list.
    if args.priority:
        args.priority = [item for sublist in args.priority for item in sublist]
    else:
        args.priority = []

    mem = intelhex.IntelHex(args.hex)

    print_header()
    parse_messages(mem, 1, args.priority)

if __name__ == "__main__":
    sys.exit(main())

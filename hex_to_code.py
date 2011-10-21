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

def print_mask(mask, value):
    low = (value >> 3) & 0xff
    high = (value << 5) & 0xff

    print '  mcp2515_write_register(RXM{0}SIDH, 0x{1:x});'.format(mask, low)
    print '  mcp2515_write_register(RXM{0}SIDL, 0x{1:x});'.format(mask, high)
    print '  mcp2515_write_register(RXM{0}EID8, 0);'.format(mask)
    print '  mcp2515_write_register(RXM{0}EID0, 0);'.format(mask)
    print ''

def print_filter(filter, value):
    low = (value >> 3) & 0xff
    high = (value << 5) & 0xff

    print '  mcp2515_write_register(RXF{0}SIDH, 0x{1:x});'.format(filter, low)
    print '  mcp2515_write_register(RXF{0}SIDL, 0x{1:x});'.format(filter, high)
    print '  mcp2515_write_register(RXF{0}EID8, 0);'.format(filter)
    print '  mcp2515_write_register(RXF{0}EID0, 0);'.format(filter)
    print ''

def create_filter_code(ids, priority):
    print 'void'
    print 'setup_filters() {'

    priority_ids = [id_mapping[p] for p in priority if p in id_mapping]
    remaining_ids = [i for i in ids if i not in priority_ids]
    all_ids = priority_ids + remaining_ids

    if len(ids) <= 2:
        print '  // Only one or two messages, one for each buffer.'
        print '  // First, turn on filtering.'
        if len(ids) == 1:
            print '  // Enable overflow.'
            print ('  mcp2515_write_register(RXB0CTRL, '
                   '(0 << RXM1) | (1 << RXM0) | (1 << BUKT));')
        else:
            print ('  mcp2515_write_register(RXB0CTRL, '
                   '(0 << RXM1) | (1 << RXM0));')
	print '  mcp2515_write_register(RXB1CTRL, (0 << RXM1) | (1 << RXM0));'
        print ''
        print '  // Now, set up the masks and filters.'

        print_mask(0, 0x7ff)
        print_filter(0, all_ids[0])
        # XXX I suspect all 1s won't match anything...
        print_filter(1, 0x7ff)

        print_mask(1, 0x7ff)
        if len(ids) == 1:
            # We don't want to match anything on this filter.
            print_filter(2, 0x7ff)
        else:
            print_filter(2, all_ids[1])
        print_filter(3, 0x7ff)
        print_filter(4, 0x7ff)
        print_filter(5, 0x7ff)

    elif len(ids) <= 6:
        print '  // 6 or fewer message ids filter them all.'
	print ('  mcp2515_write_register(RXB0CTRL, '
               '(0 << RXM1) | (1 << RXM0));')
	print '  mcp2515_write_register(RXB1CTRL, (0 << RXM1) | (1 << RXM0));'

        print_mask(0, 0x7ff)
        print_mask(1, 0x7ff)

        i = 0

        for id in all_ids:
            print_filter(i, id)
            i += 1

        for id in range(i, len(ids)):
            print_filter(id, 0x7ff)

    elif priority == None or len(priority) == 0:
        print '  // No filtering, just accept all and rollover.'
	print ('  mcp2515_write_register(RXB0CTRL, '
               '(1 << RXM1) | (1 << RXM0) | (1 << BUKT));')
	print '  mcp2515_write_register(RXB1CTRL, (1 << RXM1) | (1 << RXM0));'

    else:
        print '  // Filter into 0, accept all in 1.'
        print ('  mcp2515_write_register(RXB0CTRL, '
               '(0 << RXM1) | (1 << RXM0) | (1 << BUKT));')
	print '  mcp2515_write_register(RXB1CTRL, (1 << RXM1) | (1 << RXM0));'
        print ''
        print '  // Now, set up the masks and filters.'

        print_mask(0, 0x7ff)
        print_filter(0, priority[0])
        if len(priority) > 1:
            print_filter(1, priority[1])
        else:
            # XXX I suspect all 1s won't match anything...
            print_filter(1, 0x7ff)

    print '}'
    print ''

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

def print_trailer():
    print ""

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
    print_trailer()

if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python

from collections import defaultdict
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


    # These arrays can't be initialized when we create the variables or else
    # they end up in the .data portion of the compiled program, and it becomes
    # too big for the microcontroller. Initializing them at runtime gets around
    # that problem.
    print "int FILTER_MASK_COUNT = %d;" % len(masks)
    print "CanFilterMask FILTER_MASKS[%d];" % len(masks)
    print "int FILTER_COUNT = %d;" % len(all_ids)
    print "CanFilter FILTERS[%d];" % len(all_ids)

    print
    print "void initialize_filter_arrays() {"

    print "    FILTER_MASKS = {"
    for i, mask in enumerate(masks):
        print "        {%d, 0x%x}" % mask,
        if i != len(masks) - 1:
            print ","
        else:
            print "};"

    print "    FILTERS = {"
    for i, filter in enumerate(all_ids):
        # TODO what is the relationship between mask and filter? mask is a big
        # brush that catches a bunch of things, then filter does the fine
        # grained?
        print "        {%d, 0x%x, %d, %d}" % (i, all_ids[0], 1, 0),
        if i != len(all_ids) - 1:
            print ","
        else:
            print "};"
    print "}"

def parse_signal(mem, offset, message_id):
    (id, t_pos, length) = struct.unpack('<BBB', mem.gets(offset, 3))
    offset += 3
    transform = (t_pos & 1 << 7) != 0
    position = t_pos & ~(1 << 7)
    if transform:
        (off, factor) = struct.unpack('<ff', mem.gets(offset, 8))
        offset += 8
    else:
        (off, factor) = (0.0, 1.0)

    id_mapping[id] = message_id
    signal_string =  "{%d, %s, %d, %d, %s, %d, %d}" % (id, "\"\"", position,
            length, str(transform).lower(), factor, off)
    return offset, signal_string


def parse_messages(mem, offset, priority=None):
    print "void decode_can_message(int id, uint8_t* data) {"

    messages = defaultdict(list)
    signal_strings = []
    ids = []
    while offset < len(mem):
        (id, num) = struct.unpack('<HB', mem.gets(offset, 3))
        ids.append(id)
        offset += 3
        for i in range(num):
            offset, signal_string = parse_signal(mem, offset, id)
            signal_strings.append(signal_string)
            messages[id].append(len(signal_strings) - 1)

    print "    CanSignal SIGNALS[%d] = {" % len(signal_strings)
    for i, signal_string in enumerate(signal_strings):
        print "        %s" % signal_string,
        if i != len(signal_strings) - 1:
            print ","
        else:
            print "};"
    print

    print "    switch (id) {"
    for message_id, signal_indices in messages.iteritems():
        print '    case {0}:'.format(message_id)
        for signal_index in signal_indices:
            print "        decode_can_signal(data, &SIGNALS[%d]);" % (
                    signal_index)
        print "        break;"
    print "    }"
    print "}\n"

    # Create a set of filters.
    create_filter_code(ids, priority)

def print_header():
    print "#include \"canutil.h\"\n"

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

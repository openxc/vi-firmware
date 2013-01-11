#!/usr/bin/env python

from __future__ import print_function
import itertools
import sys
import argparse
import json

def parse_options():
    parser = argparse.ArgumentParser(description="Convert decimal message IDs "
            "in a JSON mapping file to hex")
    json_files = parser.add_argument("-j", "--json",
            action="append",
        type=str,
        nargs='*',
            dest="json_files",
            metavar="FILE")

    arguments = parser.parse_args()

    if not arguments.json_files:
        raise argparse.ArgumentError(json_files,
                "Must specify at least one JSON file.")

    return arguments

def main():
    arguments = parse_options()

    filenames = arguments.json_files
    if not isinstance(filenames, list):
        filenames = [filenames]
    else:
        filenames = itertools.chain(*filenames)

    for filename in filenames:
        with open(filename) as json_file:
            try:
                data = json.load(json_file)
            except ValueError as e:
                sys.stderr.write(
                        "ERROR: %s does not contain valid JSON: \n%s\n"
                        % (filename, e))
                sys.exit(1)

        for bus in data.values():
            new_messages = {}
            for message_id, message in bus.get('messages', {}).items():
                new_messages["0x%x" % int(message_id, 0)] = message
            if len(new_messages) > 0:
                bus['messages'] = new_messages

        with open(filename, 'w') as json_file:
            json.dump(data, json_file, indent=4, separators=(',', ': '),
                    sort_keys=True)


if __name__ == "__main__":
    sys.exit(main())

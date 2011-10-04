#!/usr/bin/env python

import sys
import argparse
import intelhex

def print_header():
    print "#include <EEPROM.h>"
    print ""
    print "void loop() {"
    print "}"
    print ""
    print "void setup () {"

def print_mem(mem):
    for i in range(len(mem)):
        print "  EEPROM.write({0}, {1});".format(i, mem[i])

def print_trailer():
    print ""
    print "  pinMode(13, OUTPUT);"
    print "  digitalWrite(13, HIGH);"
    print "}"

def main(argv=None):
    parser = argparse.ArgumentParser(description='Convert hex file to Arduino '
                                     'sketch.')
    parser.add_argument('hex', default='dump.hex', help='Name of hex file')

    args = parser.parse_args(argv)
    
    mem = intelhex.IntelHex(args.hex)

    print_header()
    print_mem(mem)
    print_trailer()

if __name__ == "__main__":
    sys.exit(main())

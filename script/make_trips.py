#!/usr/bin/env python

import sys
from xml.etree import ElementTree as ET
import json
import re
import datetime

class DestFileCreator:
    nextTrip = 1

    def __init__(self, firstTrip):
        self.nextTrip = firstTrip

    def next_dest(self, data_file):
        newName = "Trip" + str(self.nextTrip).zfill(3) + "-" + data_file
        self.nextTrip = self.nextTrip + 1
        print "Starting ",
        print newName
        return open(newName, "w")

def get_next_file(trace_file):
    numbers_str = re.findall(r'[0-9]+', trace_file)

    numbers_int = map(int, numbers_str)

    oldFile = datetime.datetime(numbers_int[0], numbers_int[1], numbers_int[2], numbers_int[3])

    dtime = datetime.timedelta(hours=1)
    
    oldFile = oldFile + dtime

    filename = str(oldFile.date())
    filename += "-"
    if(oldFile.hour < 10):
        filename += "0"
    filename += str(oldFile.hour)
    filename += ".json"

    return filename

def compile_trip(trace_file, tripNum):
    dataFileValid = True
    lastTimeStamp = 0.0
    currentTimeStamp = 0
    destFileGen = DestFileCreator(tripNum)
    errorCount = 0
    lineCount = 0

    destinationFile = destFileGen.next_dest(trace_file)

    while dataFileValid is True:
        try:
            currentTraceFile = open(trace_file, "r")
        except IOError, e:
            print e
            dataFileValid = False
            destinationFile.close()
            break
        else:
            print 'Opened %s' % trace_file

            for line in currentTraceFile:
                try:
                    lineCount = lineCount + 1
                    timestamp, data = line.split(':', 1)
                    record = json.loads(data)
                except ValueError:
                    sys.stderr.write("Skipping line: %s" % data)
                    print " "
                    errorCount = errorCount + 1
                    continue
                
                if lastTimeStamp is not 0.0:
                    if (float(timestamp) - lastTimeStamp) > 600.00:  # Time is in seconds
                        print "Found a gap of ",
                        print (float(timestamp) - lastTimeStamp),
                        print " seconds.  Creating new Trip file."
                        destinationFile.close()
                        lastTimeStamp = 0.0
                        destinationFile = destFileGen.next_dest(trace_file)
                    elif (float(timestamp) - lastTimeStamp) > 1.00:  # Time is in seconds
                        print "Momentary dropout of ",
                        print (float(timestamp) - lastTimeStamp),
                        print " seconds.  Ignoring."
                lastTimeStamp = float(timestamp)
                destinationFile.write(line)
                
            if dataFileValid is True:
                currentTraceFile.close()
                trace_file = get_next_file(trace_file)

    percentBad = 100.0 * errorCount / lineCount
    print "Parsed",
    print lineCount,
    print "lines."

    print "Detected",
    print errorCount,
    print "errors."

    print percentBad,
    print "% bad data."

if __name__ == '__main__':
    if len(sys.argv) is not 3:
        print "Must provide the path to the first trace file in a trip and the trip number."
        sys.exit(1)
    
    compile_trip(sys.argv[1], int(sys.argv[2]))

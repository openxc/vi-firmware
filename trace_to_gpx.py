#!/usr/bin/env python

import sys
from xml.etree import ElementTree as ET
import json

def generate_gpx(trace_file):
    root = ET.Element("gpx")

    track = ET.SubElement(root, "trk")
    number = ET.SubElement(track, "number")
    number.text = "1"

    latitude = None
    longitude = None

    segment = ET.SubElement(track, "trkseg")
    for line in open(trace_file):
        timestamp, data = line.split(':', 1)
        record = json.loads(data)
        if record['name'] == 'latitude':
            latitude = record['value']
        elif record['name'] == 'longitude':
            longitude = record['value']

        if latitude and longitude:
            point = ET.SubElement(segment, "trkpt")
            point.set('lat', str(latitude))
            point.set('lon', str(longitude))
            latitude = longitude = None

    return ET.tostring(ET.ElementTree(root).getroot())

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print "Must provide the path to a trace file as the only argument"
        sys.exit(1)
    print generate_gpx(sys.argv[1])

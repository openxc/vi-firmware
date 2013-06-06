import sys
import collections
import os
import json

# Only works with 2 CAN buses since we are limited by 2 CAN controllers,
# and we want to be a little careful that we always expect 0x101 to be
# plugged into the CAN1 controller and 0x102 into CAN2.
VALID_BUS_ADDRESSES = (1, 2)


def fatal_error(message):
    # TODO add red color
    sys.stderr.write("ERROR: %s\n" % message)
    sys.exit(1)


def warning(message):
    # TODO add yellow color
    sys.stderr.write("WARNING: %s\n" % message)


def quacks_like_dict(object):
    """Check if object is dict-like"""
    return isinstance(object, collections.Mapping)

def quacks_like_list(object):
    """Check if object is list-like"""
    return hasattr(object, '__iter__') and hasattr(object, 'append')


def merge(a, b):
    """Merge two deep dicts non-destructively

    Uses a stack to avoid maximum recursion depth exceptions

    >>> a = {'a': 1, 'b': {1: 1, 2: 2}, 'd': 6}
    >>> b = {'c': 3, 'b': {2: 7}, 'd': {'z': [1, 2, 3]}}
    >>> c = merge(a, b)
    >>> from pprint import pprint; pprint(c)
    {'a': 1, 'b': {1: 1, 2: 7}, 'c': 3, 'd': {'z': [1, 2, 3]}}
    """
    assert quacks_like_dict(a), quacks_like_dict(b)
    dst = a.copy()

    stack = [(dst, b)]
    while stack:
        current_dst, current_src = stack.pop()
        for key in current_src:
            if key not in current_dst:
                current_dst[key] = current_src[key]
            else:
                if (quacks_like_dict(current_src[key]) and
                        quacks_like_dict(current_dst[key])):
                    stack.append((current_dst[key], current_src[key]))
                elif (quacks_like_list(current_src[key]) and
                        quacks_like_list(current_dst[key])):
                    current_dst[key].extend(current_src[key])
                else:
                    current_dst[key] = current_src[key]
    return dst


class Command(object):
    def __init__(self, generic_name, handler=None):
        self.generic_name = generic_name
        self.handler = handler

    def __str__(self):
        return "{ \"%s\", %s }," % (self.generic_name, self.handler)


def find_file(filename, search_paths):
    for search_path in search_paths:
        full_path = "%s/%s" % (search_path, filename)
        if os.path.exists(full_path):
            return full_path
    fatal_error("Unable to find '%s' in search paths (%s)" % (
            filename, search_paths))


def load_json_from_search_path(filename, search_paths):
    with open(find_file(filename, search_paths)) as json_file:
        try:
            data = json.load(json_file)
        except ValueError as e:
            fatal_error("%s does not contain valid JSON: \n%s\n" %
                    (filename, e))
        else:
            return data


class Message(object):
    def __init__(self, buses, bus_name, id, name,
            handler=None):
        self.bus_name = bus_name
        self.buses = buses
        self.id = int(id, 0)
        self.name = name
        self.handler = handler
        self.signals = []

    def __str__(self):
        bus_index = self._lookup_bus_index(self.buses, self.bus_name)
        if bus_index is not None:
            return "{&CAN_BUSES[%d][%d], 0x%x}, // %s" % (self.message_set_index,
                    bus_index, self.id, self.name)
        else:
            warning("Bus address '%s' is invalid, only %s are allowed - message 0x%x will be disabled\n" %
                    (self.bus_name, VALID_BUS_ADDRESSES, self.id))
        return ""

    @staticmethod
    def _lookup_bus_index(buses, bus_name):
        if bus_name in buses and 'controller' in buses[bus_name]:
            for index, candidate_bus_address in enumerate(VALID_BUS_ADDRESSES):
                if candidate_bus_address == buses[bus_name]['controller']:
                    return index
        return None


class Signal(object):
    def __init__(self, message_set=None, message=None, name=None,
            generic_name=None, bit_position=None, bit_size=None, factor=1,
            offset=0, min_value=0.0, max_value=0.0, handler=None, ignore=False,
            states=None, send_frequency=1, send_same=True, writable=False,
            write_handler=None):
        self.message_set = message_set
        self.message = message
        self.name = name
        self.generic_name = generic_name
        self.bit_position = bit_position
        self.bit_size = bit_size
        self.factor = factor
        self.offset = offset
        self.min_value = min_value
        self.max_value = max_value
        self.handler = handler
        self.writable = writable
        self.write_handler = write_handler
        if ignore:
            self.handler = "ignoreHandler"
        self.array_index = 0
        # the frequency determines how often the message should be propagated. a
        # frequency of 1 means that every time the signal it is received we will
        # try to handle it. a frequency of 2 means that every other signal
        # will be handled (and the other half is ignored). This is useful for
        # trimming down the data rate of the stream over USB.
        self.send_frequency = send_frequency
        self.send_same = send_same

        if self.send_same is False and self.send_frequency != 1:
            warning("Signal %s combines send_same and " % self.generic_name +
                    "send_frequency - this is not recommended")
        self.states = states or []
        if len(self.states) > 0 and self.handler is None:
            self.handler = "stateHandler"

    @classmethod
    def from_xml_node(cls, node):
        """Construct a Signal instance from an XML node exported from a Vector
        CANoe .dbc file."""
        signal = Signal(name=node.find("Name").text,
                bit_position=int(node.find("Bitposition").text),
                bit_size=int(node.find("Bitsize").text),
                factor=float(node.find("Factor").text),
                offset=float(node.find("Offset").text),
                min_value=float(node.find("Minimum").text),
                max_value=float(node.find("Maximum").text))

        # Invert the bit index to match the Excel mapping.
        signal.bit_position = Signal._invert_bit_index(signal.bit_position,
                signal.bit_size)
        return signal

    def to_dict(self):
        return {"generic_name": self.generic_name,
                "bit_position": self.bit_position,
                "bit_size": self.bit_size,
                "factor": self.factor,
                "offset": self.offset,
                "min_value": self.min_value,
                "max_value": self.max_value}

    def validate(self):
        if self.bit_position == None or self.bit_size == None:
            warning("%s (generic name: %s) is incomplete\n" %
                    (self.name, self.generic_name))
            return False
        return True

    @classmethod
    def _invert_bit_index(cls, i, l):
        (b, r) = divmod(i, 8)
        end = (8 * b) + (7 - r)
        return(end - l + 1)

    @staticmethod
    def _lookupMessageIndex(message_set, message):
        for i, candidate in enumerate(message_set.all_messages()):
            if candidate.id == message.id:
                return i

    def __str__(self):
        result =  ("{&CAN_MESSAGES[%d][%d], \"%s\", %s, %d, %f, %f, %f, %f, "
                    "%d, %s, false, " % (
                self.message_set_index,
                self._lookupMessageIndex(self.message_set, self.message),
                self.generic_name, self.bit_position, self.bit_size,
                self.factor, self.offset, self.min_value, self.max_value,
                self.send_frequency, str(self.send_same).lower()))
        if len(self.states) > 0:
            result += "SIGNAL_STATES[%d][%d], %d" % (self.message_set_index,
                    self.states_index, len(self.states))
        else:
            result += "NULL, 0"
        result += ", %s, %s" % (str(self.writable).lower(),
                self.write_handler or "NULL")
        result += "}, // %s" % self.name
        return result


class SignalState(object):
    def __init__(self, value, name):
        self.value = value
        self.name = name

    def __str__(self):
        return "{%d, \"%s\"}" % (self.value, self.name)

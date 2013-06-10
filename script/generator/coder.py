"""
C++ source code generator for the vehicle interface firmware.
"""
import os
import sys
import operator

from common import warning, find_file

GENERATED_CODE_VERSION = "4.0-dev"
MAX_SIGNAL_STATES = 12
base_path = os.path.dirname(sys.argv[0])


class CodeGenerator(object):
    """This class is used to build an implementation of the signals.h functions
    from one or more CAN message sets. The message sets must already be read
    into memory and parsed.
    """
    def __init__(self, search_paths):
        self.search_paths = search_paths
        self.message_sets = []

    def build_source(self):
        lines = []
        lines.extend(self._build_header())
        lines.extend(self._build_extra_sources())
        lines.extend(self._build_message_sets())
        lines.extend(self._build_buses())
        lines.extend(self._build_messages())
        lines.extend(self._build_signal_states())
        lines.extend(self._build_signals())
        lines.extend(self._build_initializers())
        lines.extend(self._build_loop())
        lines.extend(self._build_commands())
        lines.extend(self._build_decoder())
        lines.extend(self._build_filters())

        with open("%s/signals.cpp.footer" % base_path) as footer:
            lines.append("")
            lines.append(footer.read())

        return '\n'.join(lines)

    @property
    def sorted_message_sets(self):
        return sorted(self.message_sets, key=operator.attrgetter('name'))

    def _max_command_count(self):
        if len(self.message_sets) == 0:
            return 0

        return max(len(list(message_set.active_commands()))
                for message_set in self.message_sets)

    def _max_message_count(self):
        if len(self.message_sets) == 0:
            return 0
        return max(len(list(message_set.active_messages()))
                for message_set in self.message_sets)

    def _max_signal_count(self):
        if len(self.message_sets) == 0:
            return 0

        return max(len(list(message_set.active_signals()))
                for message_set in self.message_sets)

    def _build_header(self):
        with open("%s/signals.cpp.header" % base_path) as header:
            return [header.read().format(GENERATED_CODE_VERSION)]

    def _build_extra_sources(self):
        lines = []
        for i, message_set in enumerate(self.sorted_message_sets):
            for extra_source_filename in message_set.extra_sources:
                with open(find_file(extra_source_filename, self.search_paths)
                        ) as extra_source_file:
                    lines.append(extra_source_file.read())
        return lines

    def _build_message_set(self, index, message_set):
        return "    { %d, \"%s\", %d, %d, %d }," % (index, message_set.name,
                len(message_set.buses), len(list(message_set.active_messages())),
                len(list(message_set.active_signals())))

    def _build_bus_struct(self, bus_address, bus, bus_number):
        result = """        {{ {bus_speed}, {bus_address}, can{bus_number},
            #ifdef __PIC32__
            handleCan{bus_number}Interrupt,
            #endif // __PIC32__
        }},"""
        return result.format(bus_speed=bus['speed'], bus_address=bus_address,
                bus_number=bus_number)

    def _build_messages(self):
        lines = []
        lines.append("const int MAX_MESSAGE_COUNT = %d;" %
                self._max_message_count())
        lines.append("CanMessage CAN_MESSAGES[][MAX_MESSAGE_COUNT] = {")

        def block(message_set, message_set_index=None):
            lines = []
            for message_index, message in enumerate(message_set.all_messages()):
                if not message.enabled:
                    warning("Message %s (0x%x) is disabled, excluding from source" %
                            (message.name, message.id))
                    continue
                message.message_set_index = message_set_index
                message.array_index = message_index
                lines.append("        %s" % message)
            return lines

        lines.extend(self._message_set_lister(block))

        lines.append("};")
        lines.append("")
        return lines

    def _build_message_sets(self):
        lines = []
        lines.append("const int MESSAGE_SET_COUNT = %d;" %
                len(self.message_sets))
        lines.append("CanMessageSet MESSAGE_SETS[MESSAGE_SET_COUNT] = {")
        for i, message_set in enumerate(self.sorted_message_sets):
            lines.append(self._build_message_set(i, message_set))
        lines.append("};")
        lines.append("")
        return lines

    def _build_buses(self):
        lines = []
        lines.append("const int MAX_CAN_BUS_COUNT = 2;")
        lines.append("CanBus CAN_BUSES[][MAX_CAN_BUS_COUNT] = {")

        def block(message_set, **kwargs):
            lines = []
            for bus_number, (bus_address, bus) in enumerate(
                    message_set.valid_buses()):
                lines.append(self._build_bus_struct(bus_address, bus,
                    bus_number + 1))
                lines.append("")
            return lines

        lines.extend(self._message_set_lister(block))
        lines.append("};")
        lines.append("")

        return lines

    def _build_filters(self):
        # These arrays can't be initialized when we create the variables or else
        # they end up in the .data portion of the compiled program, and it
        # becomes too big for the microcontroller. Initializing them at runtime
        # gets around that problem.
        lines = []
        lines.append("CanFilter FILTERS[MAX_MESSAGE_COUNT];")

        lines.append("")
        lines.append("CanFilter* openxc::signals::initializeFilters("
                "uint64_t address, int* count) {")

        def block(message_set_index, message_set):
            lines = []
            lines.append("        switch(address) {")
            for bus_address, bus in message_set.valid_buses():
                lines.append("        case %s:" % bus_address)
                lines.append("            *count = %d;" % len(bus['messages']))
                for i, message in enumerate(sorted(bus['messages'],
                        key=operator.attrgetter('id'))):
                    lines.append("            FILTERS[%d] = {%d, 0x%x, %d};" % (
                            i, i, message.id, 1))
                lines.append("            break;")
            lines.append("        }")
            return lines

        lines.extend(self._message_set_switcher(block))
        lines.append("    return FILTERS;")
        lines.append("}")
        return lines

    def _build_signal_states(self):
        lines = []
        lines.append("const int MAX_SIGNAL_STATES = %d;" % MAX_SIGNAL_STATES)
        lines.append("const int MAX_SIGNAL_COUNT = %d;" %
                self._max_signal_count())
        lines.append("CanSignalState SIGNAL_STATES[]"
                "[MAX_SIGNAL_COUNT][MAX_SIGNAL_STATES] = {")

        def block(message_set, **kwargs):
            states_index = 0
            lines = []
            for signal in message_set.active_signals():
                if len(signal.states) > 0:
                    line = "        { "
                    for state_count, state in enumerate(signal.sorted_states):
                        if state_count >= MAX_SIGNAL_STATES:
                            warning("Ignoring anything beyond %d states for %s" %
                                    (MAX_SIGNAL_STATES, signal.generic_name))
                            break
                        line += "%s, " % state
                    line += "},"
                    lines.append(line)
                    signal.states_index = states_index
                    states_index += 1
            return lines

        lines.extend(self._message_set_lister(block))

        lines.append("};")
        lines.append("")

        return lines

    def _message_set_lister(self, block, indent=4):
        lines = []
        whitespace = " " * indent
        for message_set_index, message_set in enumerate(
                self.sorted_message_sets):
            lines.append(whitespace + "{ // message set: %s" % message_set.name)
            lines.extend(block(message_set,
                    message_set_index=message_set_index))
            lines.append(whitespace + "},")
        return lines

    def _message_set_switcher(self, block, indent=4):
        lines = []
        whitespace = " " * indent
        lines.append(whitespace + "switch(getConfiguration()"
                "->messageSetIndex) {")
        for message_set_index, message_set in enumerate(
                self.sorted_message_sets):
            lines.append(whitespace + "case %d: // message set: %s" % (
                    message_set_index, message_set.name))
            lines.extend(block(message_set_index, message_set))
            lines.append(whitespace * 2 + "break;")
            lines.append(whitespace + "}")
        return lines

    def _build_signals(self):
        lines = []
        lines.append("CanSignal SIGNALS[][MAX_SIGNAL_COUNT] = {")

        def block(message_set, message_set_index=None):
            lines = []
            i = 1
            for signal in message_set.all_signals():
                if not signal.enabled:
                    warning("Signal '%s' (in 0x%x) is disabled, " % (
                                signal.name, signal.message.id) +
                            "excluding from source")
                    continue
                signal.message_set_index = message_set_index
                signal.array_index = i - 1
                lines.append("        %s" % signal)
                i += 1
            return lines

        lines.extend(self._message_set_lister(block))
        lines.append("};")
        lines.append("")

        return lines

    def _build_initializers(self):
        lines = []
        lines.append("void openxc::signals::initialize() {")

        def block(message_set_index, message_set):
            return ["        %s();" % initializer
                for initializer in message_set.initializers]
        lines.extend(self._message_set_switcher(block))
        lines.append("}")
        lines.append("")
        return lines

    def _build_loop(self):
        lines = []
        lines.append("void openxc::signals::loop() {")
        def block(message_set_index, message_set):
            return ["        %s();" % looper for looper in message_set.loopers]
        lines.extend(self._message_set_switcher(block))
        lines.append("}")
        lines.append("")
        return lines

    def _build_commands(self):
        lines = []
        lines.append("const int MAX_COMMAND_COUNT = %d;" %
                self._max_command_count())
        lines.append("CanCommand COMMANDS[][MAX_COMMAND_COUNT] = {")
        def block(message_set, **kwargs):
            for command in message_set.commands:
                if not command.enabled:
                    warning("Command %s is disabled, excluding from source" %
                            command.name)
                    continue
                yield "        %s" % command
        lines.extend(self._message_set_lister(block))
        lines.append("};")
        lines.append("")

        return lines

    def _build_decoder(self):
        lines = []
        lines.append("void openxc::signals::decodeCanMessage("
                "Pipeline* pipeline, CanBus* bus, int id, uint64_t data) {")

        def block(message_set_index, message_set):
            lines = []
            lines.append("        switch(bus->address) {")
            for bus_address, bus in message_set.valid_buses():
                lines.append("        case %s:" % bus_address)
                lines.append("            switch (id) {")
                for message in sorted(bus['messages'],
                        key=operator.attrgetter('id')):
                    lines.append("            case 0x%x: // %s" % (message.id,
                            message.name))
                    if message.handler is not None:
                        lines.append(" " * 16 + "%s(id, data, SIGNALS[%d], " % (
                            message.handler, message_set_index) +
                                "getSignalCount(), pipeline);")
                    for signal in sorted((s for s in message.signals),
                            key=operator.attrgetter('generic_name')):
                        line = " " * 16
                        line += ("can::read::translateSignal(pipeline, "
                                    "&SIGNALS[%d][%d], data, " %
                                    (message_set_index, signal.array_index))
                        if signal.handler:
                            line += "&%s, " % signal.handler
                        line += ("SIGNALS[%d], getSignalCount()); // %s" % (
                            message_set_index, signal.name))
                        lines.append(line)
                    lines.append("                break;")
                lines.append("            }")
                lines.append("            break;")
            lines.append("        }")
            return lines

        lines.extend(self._message_set_switcher(block))

        if self._max_message_count() == 0:
            lines.append("    openxc::can::read::passthroughMessage(pipeline, "
                    "id, data);")

        lines.append("}")
        lines.append("")

        return lines

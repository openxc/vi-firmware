import os
import sys
import operator

from common import valid_buses, all_messages, fatal_error, warning

MAX_SIGNAL_STATES = 12
base_path = os.path.dirname(sys.argv[0])

class CodeGenerator(object):
    def __init__(self):
        self.message_sets = []

    def build_header(self):
        result = ""
        with open("%s/signals.cpp.header" % base_path) as header:
            result += header.read()

        if getattr(self, 'uses_custom_handlers', None):
            '\n'.join([result,
                "#include \"handlers.h\"",
                "using namespace openxc::signals::handlers;"])
        return result

    def _build_bus_struct(self, bus_address, bus, bus_number):
        result = """    {{ {bus_speed}, {bus_address}, can{bus_number},
        #ifdef __PIC32__
        handleCan{bus_number}Interrupt,
        #endif // __PIC32__
    }},"""
        return result.format(bus_speed=bus['speed'], bus_address=bus_address,
                bus_number=bus_number)

    def build_source(self):
        if not self.validate_messages() or not self.validate_name():
            fatal_error("unable to generate code")
        lines = [self.build_header()]

        lines.append("const int CAN_BUS_COUNT = %d;" % len(
                list(valid_buses(self.buses))))
        lines.append("CanBus CAN_BUSES[CAN_BUS_COUNT] = {")
        for bus_number, (bus_address, bus) in enumerate(
                valid_buses(self.buses)):
            lines.append(self._build_bus_struct(bus_address, bus, bus_number +
                1))
            lines.append("")

        lines.append("};")
        lines.append("")

        lines.append("const int MESSAGE_COUNT = %d;" % self._message_count)
        lines.append("CanMessage CAN_MESSAGES[MESSAGE_COUNT] = {")

        for i, message in enumerate(all_messages(self.buses)):
            message.array_index = i
            lines.append("    %s" % message)
        lines.append("};")
        lines.append("")

        lines.append("const int SIGNAL_COUNT = %d;" % self.signal_count)
        lines.append(("CanSignalState SIGNAL_STATES[SIGNAL_COUNT][%d] = {"
                % MAX_SIGNAL_STATES))

        states_index = 0
        for message in all_messages(self.buses):
            for signal in message.signals:
                if len(signal.states) > 0:
                    if states_index >= MAX_SIGNAL_STATES:
                        warning("Ignoring anything beyond %d states for %s" %
                                (MAX_SIGNAL_STATES, signal.generic_name))
                        break

                    lines.append("    {", end=' ')
                    for state in signal.states:
                        lines.append("%s," % state, end=' ')
                    lines.append("},")
                    signal.states_index = states_index
                    states_index += 1
        lines.append("};")
        lines.append("")

        lines.append("CanSignal SIGNALS[SIGNAL_COUNT] = {")

        i = 1
        for message in all_messages(self.buses):
            message.signals = sorted(message.signals,
                    key=operator.attrgetter('generic_name'))
            for signal in message.signals:
                signal.array_index = i - 1
                lines.append("    %s" % signal)
                i += 1
        lines.append("};")
        lines.append("")

        lines.append("void openxc::signals::initializeSignals() {")
        for initializer in self.initializers:
            lines.append("    %s();" % initializer)
        lines.append("}")
        lines.append("")

        lines.append("void openxc::signals::loop() {")
        for looper in self.loopers:
            lines.append("    %s();" % looper)
        lines.append("}")
        lines.append("")

        lines.append("const int COMMAND_COUNT = %d;" % self.command_count)
        lines.append("CanCommand COMMANDS[COMMAND_COUNT] = {")

        for command in self.commands:
            lines.append("    ", command)

        lines.append("};")
        lines.append("")

        with open("%s/signals.cpp.middle" % base_path) as middle:
            lines.append(middle.read())

        lines.append("const char* openxc::signals::getMessageSet() {")
        lines.append("    return \"%s\";" % self.name)
        lines.append("}")
        lines.append("")

        lines.append("void openxc::signals::decodeCanMessage(Pipeline* pipeline, "
                "CanBus* bus, int id, uint64_t data) {")
        lines.append("    switch(bus->address) {")
        for bus_address, bus in valid_buses(self.buses):
            lines.append("    case %s:" % bus_address)
            lines.append("        switch (id) {")
            for message in bus['messages']:
                lines.append("        case 0x%x: // %s" % (message.id, message.name))
                if message.handler is not None:
                    lines.append(("            %s(id, data, SIGNALS, " %
                        message.handler + "SIGNAL_COUNT, pipeline);"))
                for signal in (s for s in message.signals):
                    if signal.handler:
                        lines.append(("            can::read::translateSignal("
                                "pipeline, "
                                "&SIGNALS[%d], data, " % signal.array_index +
                                "&%s, SIGNALS, SIGNAL_COUNT); // %s" % (
                                signal.handler, signal.name)))
                    else:
                        lines.append(("            can::read::translateSignal("
                                "pipeline, "
                                "&SIGNALS[%d], " % signal.array_index +
                                "data, SIGNALS, SIGNAL_COUNT); // %s"
                                    % signal.name))
                lines.append("            break;")
            lines.append("        }")
            lines.append("        break;")
        lines.append("    }")

        if self._message_count() == 0:
            lines.append("    openxc::can::read::passthroughMessage(pipeline, id, "
                    "data);")

        lines.append("}")
        lines.append("")

        # Create a set of filters.
        lines.append(self.build_filters())
        lines.append("")
        lines.append("#endif // CAN_EMULATOR")

        return '\n'.join(lines)

    def build_filters(self):
        # These arrays can't be initialized when we create the variables or else
        # they end up in the .data portion of the compiled program, and it
        # becomes too big for the microcontroller. Initializing them at runtime
        # gets around that problem.
        lines = []
        lines.append("CanFilter FILTERS[%d];" % self._message_count())

        lines.append("")
        lines.append("CanFilter* openxc::signals::initializeFilters(uint64_t address, "
                "int* count) {")
        lines.append("    switch(address) {")
        for bus_address, bus in valid_buses(self.buses):
            lines.append("    case %s:" % bus_address)
            lines.append("        *count = %d;" % len(bus['messages']))
            for i, message in enumerate(bus['messages']):
                lines.append("        FILTERS[%d] = {%d, 0x%x, %d};" % (
                        i, i, message.id, 1))
            lines.append("        break;")
        lines.append("    }")
        lines.append("    return FILTERS;")
        lines.append("}")
        return '\n'.join(lines)

#include "interface.h"

#include "uart.h"
#include "config.h"

using openxc::config::getConfiguration;

static const char interfaceNames[][5] = {
    "USB",
    "UART",
    "NET",
    "TELT",
};

const char* openxc::interface::descriptorToString(InterfaceDescriptor* descriptor) {
    if((int)descriptor->type < sizeof(interfaceNames) / sizeof(interfaceNames[0])) {
        return interfaceNames[descriptor->type];
    }
    return "Unknown";
}

bool openxc::interface::anyConnected() {
    return openxc::interface::uart::connected(&getConfiguration()->uart) ||
            openxc::interface::usb::connected(&getConfiguration()->usb) ||
            openxc::interface::network::connected(&getConfiguration()->network);
}

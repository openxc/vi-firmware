#include "interface.h"

static const char interfaceNames[][5] = {
    "USB",
    "UART",
    "NET",
};

const char* openxc::interface::descriptorToString(InterfaceDescriptor* descriptor) {
    if(descriptor->type < sizeof(interfaceNames) / sizeof(interfaceNames[0])) {
        return interfaceNames[descriptor->type];
    }
    return "Unknown";
}

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "can/canutil.h"
#include "interface/uart.h"
#include "interface/usb.h"
#include "interface/network.h"
#include "diagnostics.h"
#include "pipeline.h"

namespace openxc {
namespace config {

typedef enum {
    JSON,
    PROTO
} OutputFormat;

typedef struct {
    int messageSetIndex;
    const char* version;
    OutputFormat outputFormat;
    bool initialized;

    openxc::interface::uart::UartDevice uart;
    openxc::interface::network::NetworkDevice network;
    openxc::interface::usb::UsbDevice usb;
    openxc::diagnostics::DiagnosticsManager diagnosticsManager;
    openxc::pipeline::Pipeline pipeline;
} Configuration;

Configuration* getConfiguration();

void getFirmwareDescriptor(char* buffer, size_t length);

} // namespace config
} // namespace openxc

#endif // _CONFIG_H_

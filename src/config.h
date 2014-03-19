#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "can/canutil.h"
#include "interface/uart.h"
#include "interface/usb.h"
#include "interface/network.h"
#include "diagnostics.h"
#include "pipeline.h"
#include <payload/payload.h>

// TODO find a good home for this
#define MAX_OUTGOING_PAYLOAD_SIZE 256
#define UART_BAUD_RATE 230400

namespace openxc {
namespace config {

typedef enum {
    OFF,
    SILENT_CAN,
    OBD2_IGNITION_CHECK,
} PowerManagement;

typedef struct {
    int messageSetIndex;
    const char* version;
    openxc::payload::PayloadFormat payloadFormat;
    bool initialized;
    bool recurringObd2Requests;
    openxc::interface::uart::UartDevice uart;
    openxc::interface::network::NetworkDevice network;
    openxc::interface::usb::UsbDevice usb;
    openxc::diagnostics::DiagnosticsManager diagnosticsManager;
    openxc::pipeline::Pipeline pipeline;
    CanBus* obd2Bus;
    PowerManagement powerManagement;
    bool sendCanAcks;
} Configuration;

Configuration* getConfiguration();

void getFirmwareDescriptor(char* buffer, size_t length);

} // namespace config
} // namespace openxc

#endif // _CONFIG_H_

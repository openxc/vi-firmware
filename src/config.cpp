#include "config.h"
#include "signals.h"

using openxc::pipeline::Pipeline;
using openxc::interface::uart::UartDevice;
using openxc::payload::PayloadFormat;

namespace usb = openxc::interface::usb;

namespace signals = openxc::signals;

void initialize(openxc::config::Configuration* config) {
    config->pipeline = {
        &config->usb,
        &config->uart,
#ifdef __USE_NETWORK__
        &config->network,
#endif // __USE_NETWORK__
    };
    config->initialized = true;
}

openxc::config::Configuration* openxc::config::getConfiguration() {
    static openxc::config::Configuration CONFIG = {
        messageSetIndex: 0,
        version: "6.0-dev",
        payloadFormat:
#ifdef USE_BINARY_OUTPUT
            PayloadFormat::PROTOBUF,
#else
            PayloadFormat::JSON,
#endif
        initialized: false,
        uart: {
            baudRate: UART_BAUD_RATE
        },
        network: {},
        usb: {
            endpoints: {
                {IN_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE,
                    usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN},
                {OUT_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE,
                    usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_OUT},
                {LOG_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE,
                    usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN},
            }
        }
    };

    if(!CONFIG.initialized) {
        initialize(&CONFIG);
    }
    return &CONFIG;
}

void openxc::config::getFirmwareDescriptor(char* buffer, size_t length) {
    snprintf(buffer, length, "%s (%s)", getConfiguration()->version,
            signals::getActiveMessageSet()->name);
}

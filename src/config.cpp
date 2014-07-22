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
        version: "6.0.4-dev",
        payloadFormat: PayloadFormat::DEFAULT_OUTPUT_FORMAT,
        recurringObd2Requests: DEFAULT_RECURRING_OBD2_REQUESTS_STATUS,
        obd2BusAddress: DEFAULT_OBD2_BUS,
        powerManagement: PowerManagement::DEFAULT_POWER_MANAGEMENT,
        sendCanAcks: DEFAULT_CAN_ACK_STATUS,
        emulatedData: DEFAULT_EMULATED_DATA_STATUS,
        uartLogging: DEFAULT_UART_LOGGING_STATUS,
        calculateMetrics: DEFAULT_METRICS_STATUS,
        desiredRunLevel: RunLevel::CAN_ONLY,
        initialized: false,
        runLevel: RunLevel::NOT_RUNNING,
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
        },
        diagnosticsManager: {},
        pipeline: {},
    };

    if(!CONFIG.initialized) {
        initialize(&CONFIG);
    }
    return &CONFIG;
}

void openxc::config::getFirmwareDescriptor(char* buffer, size_t length) {
    snprintf(buffer, length, "%s (%s)", getConfiguration()->version,
            signals::getActiveMessageSet() != NULL ?
                signals::getActiveMessageSet()->name : "default");
}

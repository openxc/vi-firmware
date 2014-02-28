#include "interface/uart.h"
#include "interface/usb.h"
#include "pipeline.h"
#include "diagnostics.h"

namespace usb = openxc::interface::usb;
namespace diagnostics = openxc::diagnostics;

using openxc::interface::uart::UartDevice;
using openxc::interface::usb::UsbDevice;
using openxc::pipeline::Pipeline;

UartDevice UART_DEVICE = {230400};

UsbDevice USB_DEVICE = {
    {
        {IN_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE, usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN},
        {OUT_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE, usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_OUT},
        {LOG_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE, usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN},
    }
};

Pipeline PIPELINE = {
    openxc::pipeline::JSON,
    &USB_DEVICE,
    &UART_DEVICE,
};

diagnostics::DiagnosticsManager DIAGNOSTICS_MANAGER;

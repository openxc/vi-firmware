#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "diagnostics.h"
#include "pipeline.h"
#include <payload/payload.h>

/* Public: The baud rate for the UART connection sending and receiving OpenXC
 * messages. The specific pins used for this connection are platform dependent -
 * see the full documentation at http://vi-firmware.openxcplatform.com/en/latest/.
 */
#define UART_BAUD_RATE 230400

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

namespace openxc {
namespace config {

/* Public: Valid power management modes for the VI.
 *
 * ALWAYS_ON - As it sounds, the VI never suspends or turns off as long as power
 *      is available
 * SILENT_CAN - The VI will suspend when no CAN bus activity has been detected
 *      for a few seconds, and will wake up when any CAN message is received.
 * OBD2_IGNITION_CHECK - Actively query for the engine and vehicle speed with
 *      OBD-II diagnostic requests and suspend the VI when both are 0 for a few
 *      seconds. The VI will wake up when the CAN bus becomes active and also
 *      periodically will wake up and re-send the requests to be able to detect
 *      when the car is powered on but hides all normal mode CAN messages. Short
 *      of having a standard ignition status message, this combination of data
 *      points should be sufficient to infer if the vehicle is active and we
 *      don't risk draining the battery (including hybrids, which may have the
 *      engine off while driving).
 */
typedef enum {
    ALWAYS_ON,
    SILENT_CAN,
    OBD2_IGNITION_CHECK,
} PowerManagement;

/* Public: Valid run levels for the VI.
 *
 * NOT_RUNNING - The VI either just woke up from suspend, or is about to go to
 *      sleep.
 * CAN_ONLY - A minimal mode to allow querying for OBD-II messages, but with all
 *      I/O interfaces disabled.
 * ALL_IO - The maximum run level, where all peripherals are enabled.
 */
typedef enum {
    NOT_RUNNING,
    CAN_ONLY,
    ALL_IO,
} RunLevel;

/* Public: Valid intefaces to use for debug logging.
 */
typedef enum {
    OFF,
    USB,
    UART,
    BOTH
} LoggingOutputInterface;

/* Public: A collection of global configuration parameters for the firmware.
 *
 * There should only be one copy of this struct in existence at runtime. You can
 * retrieve this singleton with the getConfiguration() function.
 *
 * messageSetIndex - The index of the currently active message set from the
 *      signals module.
 * version - A string describing the firmware version.
 * payloadFormat - The currently active payload format, from the payload module.
 *      This is used for both input and output.
 * recurringObd2Requests - True if the VI should automatically query for
 * supported OBD-II pids and request them at a pre-defined frequency (in the
 *      diagnostics::obd2 module).
 * obd2BusAddress - If 0, OBD-II requests will not be sent. Otherwise, they will
 *      be sent on the bus with this controller address (i.e. 1 or 2).
 * powerManagement - The active power management mode.
 * sendCanAcks - True if the CAN bus controllers should be configured to send
 *      CAN ACKs. The can module must be re-initialized after changing this
 *      value..
 * emulatedData - If true, will generate fake vehicle data and include it in the
 *      published output.
 * loggingOutput - Set the output interface used for debug logging.
 * calculateMetrics - If true, metrics on CAN bus and I/O activity will be
 *      calculated and logged. This has serious performance implications at the
 *      moment.
 * desiredRunLevel - The desired run level. If this is different from the
 *      current run level, the main loop will make the changes necessary.
 *
 * Private:
 * initialized - True of the configuration struct has been initialized.
 * runLevel - The currently active run level.
 * uart -
 * network -
 * usb -
 * diagnosticsManager -
 * pipeline -
 */
typedef struct {
    int messageSetIndex;
    const char* version;
    openxc::payload::PayloadFormat payloadFormat;
    bool recurringObd2Requests;
    uint8_t obd2BusAddress;
    PowerManagement powerManagement;
    bool sendCanAcks;
    bool emulatedData;
    LoggingOutputInterface loggingOutput;
    bool calculateMetrics;
    RunLevel desiredRunLevel;
    bool initialized;
    RunLevel runLevel;
    openxc::interface::uart::UartDevice uart;
    openxc::interface::network::NetworkDevice network;
    openxc::interface::usb::UsbDevice usb;
    openxc::telitHE910::TelitDevice *telit;
    openxc::diagnostics::DiagnosticsManager diagnosticsManager;
    openxc::pipeline::Pipeline pipeline;
    char flashHash[36];
} Configuration;

/* Public: Retrieve a singleton instance of the Configuration struct.
 */
Configuration* getConfiguration();

/* Public: Write a descriptor for the running firmware into the buffer that
 * includes the version and active message set.
 *
 * buffer - The destination buffer for the descriptor.
 * length - The length of the buffer.
 */
void getFirmwareDescriptor(char* buffer, size_t length);

} // namespace config
} // namespace openxc

#endif // _CONFIG_H_

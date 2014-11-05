#include "config.h"
#include "signals.h"

using openxc::pipeline::Pipeline;
using openxc::interface::uart::UartDevice;
using openxc::payload::PayloadFormat;

namespace usb = openxc::interface::usb;
namespace telit = openxc::telitHE910;

namespace signals = openxc::signals;

void initialize(openxc::config::Configuration* config) {
    config->pipeline = {
        &config->usb,
        &config->uart,
		&config->telit,
#ifdef __USE_NETWORK__
        &config->network,
#endif // __USE_NETWORK__
    };
	config->telit.uart = &config->uart;
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
		telit: {
			config: {
				globalPositioningSettings: {
					gpsEnable: true, 
					gpsInterval: 5000, 
					gpsEnableSignal_gps_time: false, 
					gpsEnableSignal_gps_latitude: true, 
					gpsEnableSignal_gps_longitude: true, 
					gpsEnableSignal_gps_hdop: false, 
					gpsEnableSignal_gps_altitude: true, 
					gpsEnableSignal_gps_fix: true, 
					gpsEnableSignal_gps_course: false, 
					gpsEnableSignal_gps_speed: true, 
					gpsEnableSignal_gps_speed_knots: false, 
					gpsEnableSignal_gps_date: false, 
					gpsEnableSignal_gps_nsat: false
				},
				networkOperatorSettings: {
					allowDataRoaming: true,
					operatorSelectMode: telit::AUTOMATIC,
					networkDescriptor: {
						PLMN: 0,
						networkType: telit::UTRAN
					}
				},
				networkDataSettings: {
					APN: "apn"
				},
				socketConnectSettings: {
					packetSize: 0,
					idleTimeout: 0,
					connectTimeout: 600,
					txFlushTimer: 50
				},
				serverConnectSettings: {
					host: "openxcdemo.azurewebsites.net",
					port: 80
				}
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

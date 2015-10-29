#include "config.h"
#include "signals.h"
#include "md5.h"

using openxc::pipeline::Pipeline;
using openxc::interface::uart::UartDevice;
using openxc::payload::PayloadFormat;

namespace usb = openxc::interface::usb;
namespace telit = openxc::telitHE910;

namespace signals = openxc::signals;

#ifdef TELIT_HE910_SUPPORT
static void getFlashHash(openxc::config::Configuration* config);
#endif

static void initialize(openxc::config::Configuration* config) {
    config->pipeline = {
        &config->usb,
        &config->uart,
#ifdef TELIT_HE910_SUPPORT
        config->telit,
#endif
#ifdef __USE_NETWORK__
        &config->network,
#endif // __USE_NETWORK__
    };
	#ifdef TELIT_HE910_SUPPORT
    // run flashHash
    getFlashHash(config);
	#endif
	#ifdef TELIT_HE910_SUPPORT
    config->telit->uart = &config->uart;
	#endif
    config->initialized = true;
}

// if we're going to conditionally compile our "Device" config structs, we
 // will need to declare them here (conditionally) and assign the pointers
 // on the config init call (conditionally)
#ifdef TELIT_HE910_SUPPORT
openxc::telitHE910::TelitDevice telitDevice = {
	descriptor: {
		allowRawWrites: DEFAULT_ALLOW_RAW_WRITE_UART
	},
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
			"apn"
		},
		socketConnectSettings: {
			packetSize: 0,
			idleTimeout: 0,
			connectTimeout: 150,
			txFlushTimer: 50
		},
		serverConnectSettings: {
			"openxcserverdemo.azurewebsites.net",
			port: 80
		}
	}
};
#endif

openxc::config::Configuration* openxc::config::getConfiguration() {
    static openxc::config::Configuration CONFIG = {
        messageSetIndex: 0,
        version: "7.1.0",
        payloadFormat: PayloadFormat::DEFAULT_OUTPUT_FORMAT,
        recurringObd2Requests: DEFAULT_RECURRING_OBD2_REQUESTS_STATUS,
        obd2BusAddress: DEFAULT_OBD2_BUS,
        powerManagement: PowerManagement::DEFAULT_POWER_MANAGEMENT,
        sendCanAcks: DEFAULT_CAN_ACK_STATUS,
        emulatedData: DEFAULT_EMULATED_DATA_STATUS,
        loggingOutput: DEFAULT_LOGGING_OUTPUT,
        calculateMetrics: DEFAULT_METRICS_STATUS,
        desiredRunLevel: RunLevel::CAN_ONLY,
        initialized: false,
        runLevel: RunLevel::NOT_RUNNING,
        uart: {
            descriptor: {
                allowRawWrites: DEFAULT_ALLOW_RAW_WRITE_UART
            },
            baudRate: UART_BAUD_RATE
        },
        network: {
            descriptor: {
                allowRawWrites: DEFAULT_ALLOW_RAW_WRITE_NETWORK
            }
        },
        usb: {
            descriptor: {
                allowRawWrites: DEFAULT_ALLOW_RAW_WRITE_USB
            },
            endpoints: {
                {IN_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE,
                    usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN},
                {OUT_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE,
                    usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_OUT},
                {LOG_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE,
                    usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN},
            }
        },
		#ifdef TELIT_HE910_SUPPORT
        telit: &telitDevice,
		#else
		telit: NULL,
		#endif
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

#ifdef TELIT_HE910_SUPPORT
static void getFlashHash(openxc::config::Configuration* config) {
    MD5_CTX md5Context;
    unsigned char result[16];
    MD5_Init(&md5Context);
    MD5_Update(&md5Context, (const void*)0x9D001000, (unsigned long)0x2FC);
    MD5_Update(&md5Context, (const void*)0x9D001348, (unsigned long)0x7DCB8);
    MD5_Final(result, &md5Context);
    sprintf(config->flashHash, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
    result[0], result[1], result[2], result[3],
    result[4], result[5], result[6], result[7],
    result[8], result[9], result[10], result[11],
    result[12], result[13], result[14], result[15]);
}
#endif


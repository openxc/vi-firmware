#include "interface/usb.h"
#include "can/canread.h"
#include "interface/uart.h"
#include "interface/network.h"
#include "signals.h"
#include "util/log.h"
#include "cJSON.h"
#include "pipeline.h"
#include "util/timer.h"
#include "lights.h"
#include "power.h"
#include "bluetooth.h"
#include "platform_profile.h"
#include "platform/pic32/telit_he910.h"
#include "platform/pic32/server_task.h"
#include "platform/platform.h"
#include "diagnostics.h"
#include "obd2.h"
#include "data_emulator.h"
#include "config.h"
#include "commands/commands.h"
#include "platform/pic32/nvm.h"

#ifdef RTC_SUPPORT
#include "platform/pic32/rtc.h"
#endif

namespace uart = openxc::interface::uart;
namespace network = openxc::interface::network;
namespace ble = openxc::interface::ble;
namespace fs = openxc::interface::fs;

namespace usb = openxc::interface::usb;
namespace lights = openxc::lights;
namespace can = openxc::can;
namespace platform = openxc::platform;
namespace time = openxc::util::time;
namespace signals = openxc::signals;
namespace diagnostics = openxc::diagnostics;
namespace power = openxc::power;
namespace bluetooth = openxc::bluetooth;
namespace commands = openxc::commands;
namespace config = openxc::config;
namespace telit = openxc::telitHE910;
namespace server_task = openxc::server_task;
namespace nvm = openxc::nvm;

using openxc::util::log::debug;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::signals::getSignals;
using openxc::signals::getMessages;
using openxc::signals::getMessageCount;
using openxc::signals::getSignalCount;
using openxc::pipeline::Pipeline;
using openxc::config::getConfiguration;
using openxc::config::PowerManagement;
using openxc::config::RunLevel;

static bool BUS_WAS_ACTIVE;
static bool SUSPENDED;

/* Public: Update the color and status of a board's light that shows the output
 * interface status. This function is intended to be called each time through
 * the main program loop.
 */
void updateInterfaceLight() {
	//Interface connected = green led enabled/attached
	//Interface disconnected = green led disabled
	
    #ifdef TELIT_HE910_SUPPORT
    if(telit::connected(getConfiguration()->telit)) {
        lights::enable(lights::LIGHT_A, lights::COLORS.green);
    }
    #elif defined CROSSCHASM_C5_COMMON
    if(getConfiguration()->usb.configured ||	
	#if defined CROSSCHASM_C5_BLE
		ble::connected(getConfiguration()->ble) ||
	#endif	
		uart::connected(&getConfiguration()->uart)) {
        lights::enable(lights::LIGHT_A, lights::COLORS.green);
    }else {
		lights::disable(lights::LIGHT_A);
    } 
    #else
    if(uart::connected(&getConfiguration()->uart)) {
        lights::enable(lights::LIGHT_B, lights::COLORS.blue);
    }
    else if(getConfiguration()->usb.configured){ //if either of the interface are connected
        lights::enable(lights::LIGHT_B, lights::COLORS.green);
    } else {
       lights::disable(lights::LIGHT_B);
    }     
    #endif

}

/* Public: Update the color and status of a board's light that shows the status
 * of the CAN bus. This function is intended to be called each time through the
 * main program loop.
 */
void checkBusActivity() {
    bool busActive = false;
    for(int i = 0; i < getCanBusCount(); i++) {
        busActive = busActive || can::busActive(&getCanBuses()[i]);
    }

    if(!BUS_WAS_ACTIVE && busActive) {
        debug("CAN woke up");
        if(getConfiguration()->powerManagement !=
                PowerManagement::OBD2_IGNITION_CHECK) {
            // If we are letting the OBD2 ignition check control power, don't go
            // into ALL_IO just yet - we may have received an OBD-II response
            // saying the engine RPM and vehicle speed are both 0, and we want
            // to go back to sleep. In SILENT_CAN power mode it defaults to
            // ALL_IO at initialization, so this is just a backup.
            // getConfiguration()->desiredRunLevel = RunLevel::ALL_IO;
        }
		#ifdef CROSSCHASM_C5_COMMON
			lights::enable(lights::LIGHT_B, lights::COLORS.blue); //enable red led
		#else
			lights::enable(lights::LIGHT_A, lights::COLORS.blue);
		#endif
    
        BUS_WAS_ACTIVE = true;
        SUSPENDED = false;
    } else if(!busActive && (BUS_WAS_ACTIVE || (time::uptimeMs() >
            (unsigned long)openxc::can::CAN_ACTIVE_TIMEOUT_S * 1000 &&
            !SUSPENDED))) {
        debug("CAN is quiet");
        #ifdef CROSSCHASM_C5_COMMON
			lights::enable(lights::LIGHT_C, lights::COLORS.red); //enable red led
		#else
			lights::enable(lights::LIGHT_A, lights::COLORS.red);
        #endif  
		
        
        
        SUSPENDED = true;
        BUS_WAS_ACTIVE = false;
        #ifdef FS_SUPPORT
        if(fs::getmode() !=  FS_STATE::USB_CONNECTED){
        #endif        
        if(getConfiguration()->powerManagement != PowerManagement::ALWAYS_ON) {
            // stay awake at least CAN_ACTIVE_TIMEOUT_S after power on
        #ifdef RTC_SUPPORT
        rtc_timer_ms_deinit();
        #endif
            platform::suspend(&getConfiguration()->pipeline);
        }
#ifdef FS_SUPPORT
        }
#endif
    }
}

void initializeAllCan() {
    for(int i = 0; i < getCanBusCount(); i++) {
        CanBus* bus = &(getCanBuses()[i]);

        bool writable = bus->rawWritable ||
            can::signalsWritable(bus, getSignals(), getSignalCount());
        if(getConfiguration()->sendCanAcks) {
            writable = true;
        }
        can::initialize(bus, writable, getCanBuses(), getCanBusCount());
    }
}


/*
 * Check to see if a packet has been received. If so, read the packet and print
 * the packet payload to the uart monitor.
 */
void receiveCan(Pipeline* pipeline, CanBus* bus) {
    if(!QUEUE_EMPTY(CanMessage, &bus->receiveQueue)) {
        CanMessage message = QUEUE_POP(CanMessage, &bus->receiveQueue);
        signals::decodeCanMessage(pipeline, bus, &message);
        if(bus->passthroughCanMessages) {
            openxc::can::read::passthroughMessage(bus, &message, getMessages(),
                    getMessageCount(), pipeline);
        }
        bus->lastMessageReceived = time::systemTimeMs();
        ++bus->messagesReceived;

        diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager,
                bus, &message, pipeline);
    }
}

void initializeIO() {
    debug("Moving to ALL I/O runlevel");
    #ifdef RTC_SUPPORT
    RTC_Init();
    #endif    
    
    #ifdef FS_SUPPORT    
    fs::initialize(getConfiguration()->fs);
    #endif    

    usb::initialize(&getConfiguration()->usb);    
    uart::initialize(&getConfiguration()->uart); 
    
    #ifdef BLE_SUPPORT
    ble::initialize(getConfiguration()->ble);
    #endif
    #ifdef BLUETOOTH_SUPPORT
    bluetooth::start(&getConfiguration()->uart);
    #endif
    network::initialize(&getConfiguration()->network);
    getConfiguration()->runLevel = RunLevel::ALL_IO;


}

void initializeVehicleInterface() {
    #ifdef TELIT_HE910_SUPPORT
    nvm::initialize();
    #endif
    platform::initialize();
    openxc::util::log::initialize();
    time::initialize();
    power::initialize();
    lights::initialize();

    srand(time::systemTimeMs());
    initializeAllCan();

    char descriptor[128];
    config::getFirmwareDescriptor(descriptor, sizeof(descriptor));
    debug("Performing minimal initialization for %s", descriptor);
    BUS_WAS_ACTIVE = false;

    diagnostics::initialize(&getConfiguration()->diagnosticsManager,
            getCanBuses(), getCanBusCount(),
            getConfiguration()->obd2BusAddress);
    signals::initialize(&getConfiguration()->diagnosticsManager);
    getConfiguration()->runLevel = RunLevel::CAN_ONLY;

    if(getConfiguration()->powerManagement ==
            PowerManagement::OBD2_IGNITION_CHECK) {
        getConfiguration()->desiredRunLevel = RunLevel::CAN_ONLY;
    } else {
        getConfiguration()->desiredRunLevel = RunLevel::ALL_IO;
        initializeIO();
    }

    // If we don't delay a little bit, time::elapsed seems to return true no
    // matter what for DEBUG=0 builds.
    time::delayMs(500);
    
}

void firmwareLoop() {
    
    if(getConfiguration()->runLevel != RunLevel::ALL_IO &&
            getConfiguration()->desiredRunLevel == RunLevel::ALL_IO) {
        initializeIO();
        
    }
    for(int i = 0; i < getCanBusCount(); i++) {
        // In normal operation, if no output interface is enabled/attached (e.g.
        // no USB or Bluetooth, the loop will stall here. Deep down in
        // receiveCan when it tries to append messages to the queue it will
        // reach a point where it tries to flush the (full) queue. Since nothing
        // is attached, that will just keep timing out. Just be aware that if
        // you need to modify the firmware to not use any interfaces, you'll
        // have to change that or enable the flush functionality to write to
        // your desired output interface.
        CanBus* bus = &(getCanBuses()[i]);
        receiveCan(&getConfiguration()->pipeline, bus);
        diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, bus);
        
    }

    diagnostics::obd2::loop(&getConfiguration()->diagnosticsManager);
    

    if(getConfiguration()->runLevel == RunLevel::ALL_IO) {
        usb::read(&getConfiguration()->usb, usb::handleIncomingMessage);
        #ifdef TELIT_HE910_SUPPORT
        telit::connectionManager(getConfiguration()->telit);
        if(telit::connected(getConfiguration()->telit)) {
            if(getConfiguration()->telit->config.globalPositioningSettings.gpsEnable) {
                telit::getGPSLocation();
            }
            server_task::firmwareCheck(getConfiguration()->telit);
            server_task::flushDataBuffer(getConfiguration()->telit);
            server_task::commandCheck(getConfiguration()->telit);
        }
        #elif defined BLE_SUPPORT
        ble::read(getConfiguration()->ble); 
        #else
        uart::read(&getConfiguration()->uart, uart::handleIncomingMessage);
        #endif
        network::read(&getConfiguration()->network,
                network::handleIncomingMessage);
                
    }

    for(int i = 0; i < getCanBusCount(); i++) {
        can::write::flushOutgoingCanMessageQueue(&getCanBuses()[i]);
    }

    checkBusActivity();
    if(getConfiguration()->runLevel == RunLevel::ALL_IO) {
        updateInterfaceLight();
    }

    signals::loop();

    can::logBusStatistics(getCanBuses(), getCanBusCount());
    openxc::pipeline::logStatistics(&getConfiguration()->pipeline);

    if(getConfiguration()->emulatedData) {
        static bool connected = false;
        if(!connected && openxc::interface::anyConnected()) {
            connected = true;
            openxc::emulator::restart();
        } else if(connected && !openxc::interface::anyConnected()) {
            connected = false;
        }

        if(connected) {
            openxc::emulator::generateFakeMeasurements(
                    &getConfiguration()->pipeline);
        }
    }
    #ifdef FS_SUPPORT
    fs::manager(getConfiguration()->fs);
    #endif
    #ifdef RTC_SUPPORT
    rtc_task();
    #endif
    openxc::pipeline::process(&getConfiguration()->pipeline);
    
}

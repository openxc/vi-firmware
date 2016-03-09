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


using openxc::pipeline::Pipeline;
using openxc::config::getConfiguration;
using openxc::config::PowerManagement;
using openxc::config::RunLevel;


//ERROR CODES
#define SD_CARD_MOUNT_ERROR 1
#define RTC_INIT_ERROR      2
#define BLE_INIT_ERROR		3
 

void enter_fault_state(uint8_t fc){
	
	while(fc--){
		lights::enable(lights::LIGHT_A, lights::COLORS.red);
		time::delayMs(1000);
		lights::disable(lights::LIGHT_A);
		time::delayMs(1000);
	}
	lights::enable(lights::LIGHT_A, lights::COLORS.red);
	while(1); //wait here forever
}


void initializeTestInterface(void){
	platform::initialize();
	time::initialize();
    power::initialize();
    lights::initialize(); 
#ifdef RTC_SUPPORT	 
	if(RTC_Init() == 0){
		enter_fault_state(RTC_INIT_ERROR);
	}
#endif	
#ifdef BLE_SUPPORT
	if(ble::initialize(getConfiguration()->ble) == 0){
		enter_fault_state(BLE_INIT_ERROR);
	}
#endif

#ifdef FS_SUPPORT
	if(fs::initialize(getConfiguration()->fs) == 0){
		enter_fault_state(SD_CARD_MOUNT_ERROR);
	}
#endif	
	
}

void testfirmwareLoop(void){
	lights::enable(lights::LIGHT_C, lights::COLORS.green);
}

#include "interface/ble.h"

#include <stddef.h>

#include "util/log.h"
#include "config.h"

using openxc::util::log::debug;

void openxc::interface::ble::initializeCommon(BleDevice* device) {
    if(device != NULL) {
        debug("Initializing Bluetooth Low Energy common...");
        QUEUE_INIT(uint8_t,(QUEUE_TYPE(uint8_t)* ) &device->receiveQueue);//messages received over BLE characteristic write
        QUEUE_INIT(uint8_t,(QUEUE_TYPE(uint8_t)* ) &device->sendQueue);
        device->descriptor.type = InterfaceType::BLE;
    }
}

void openxc::interface::ble::deinitializeCommon(BleDevice* device) {
   
}

size_t    openxc::interface::ble::handleIncomingMessage(uint8_t payload[], size_t length) { //to go in root folder maybe
    return openxc::commands::handleIncomingMessage(payload, length,
            &config::getConfiguration()->ble->descriptor); 
            
    
}


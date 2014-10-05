#include "commands/can_message_write_command.h"

#include "config.h"
#include "diagnostics.h"
#include "util/log.h"
#include "config.h"
#include "pb_decode.h"
#include <payload/payload.h>
#include "signals.h"
#include <can/canutil.h>
#include <bitfield/bitfield.h>
#include <limits.h>

using openxc::util::log::debug;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::can::lookupBus;

namespace can = openxc::can;
namespace interface = openxc::interface;

bool openxc::commands::handleRaw(openxc_VehicleMessage* message,
        openxc::interface::InterfaceDescriptor* sourceInterfaceDescriptor) {
    bool status = true;
    if(message->has_raw_message) {
        openxc_RawMessage* rawMessage = &message->raw_message;
        CanBus* matchingBus = NULL;
        if(rawMessage->has_bus) {
            matchingBus = lookupBus(rawMessage->bus, getCanBuses(), getCanBusCount());
        } else if(getCanBusCount() > 0) {
            matchingBus = &getCanBuses()[0];
            debug("No bus specified for write, using the first active: %d", matchingBus->address);
        }

        if(!sourceInterfaceDescriptor->allowRawWrites) {
            debug("Direct CAN message writes not allowed from %s interface",
                    interface::descriptorToString(sourceInterfaceDescriptor));
            status = false;
        } else if(matchingBus == NULL) {
            debug("No matching active bus for requested address: %d",
                    rawMessage->bus);
            status = false;
        } else if(matchingBus->rawWritable) {
            uint8_t size = rawMessage->data.size;
            CanMessage message = {
                id: rawMessage->message_id,
                format: rawMessage->message_id > 2047 ? CanMessageFormat::EXTENDED : CanMessageFormat::STANDARD
            };
            memcpy(message.data, rawMessage->data.bytes, size);
            message.length = size;
            can::write::enqueueMessage(matchingBus, &message);
        } else {
            debug("Raw CAN writes not allowed for bus %d", matchingBus->address);
            status = false;
        }
    }
    return status;
}

bool openxc::commands::validateRaw(openxc_VehicleMessage* message) {
    bool valid = true;
    if(message->has_type && message->type == openxc_VehicleMessage_Type_RAW &&
            message->has_raw_message) {
        openxc_RawMessage* raw = &message->raw_message;
        if(!raw->has_message_id) {
            valid = false;
            debug("Write request is malformed, missing id");
        }

        if(!raw->has_data) {
            valid = false;
            debug("Raw write request for 0x%02x missing data", raw->message_id);
        }
    } else {
        valid = false;
    }
    return valid;
}

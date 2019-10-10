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

bool openxc::commands::handleCan(openxc_VehicleMessage* message,
        openxc::interface::InterfaceDescriptor* sourceInterfaceDescriptor) {
    bool status = true;
    //if(message->has_can_message) {
    if(message->type == openxc_VehicleMessage_Type_CAN) {
        openxc_CanMessage* canMessage = &message->can_message;
        CanBus* matchingBus = NULL;
        //if(canMessage->has_bus) {
        if(canMessage->bus > 0) {
            matchingBus = lookupBus(canMessage->bus, getCanBuses(), getCanBusCount());
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
                    canMessage->bus);
            status = false;
        } else if(matchingBus->rawWritable) {
            uint8_t size = canMessage->data.size;
            CanMessageFormat format;

            //if(canMessage->has_frame_format) {
            if(canMessage->frame_format != openxc_CanMessage_FrameFormat_UNUSED) {
                format = canMessage->frame_format ==
                        openxc_CanMessage_FrameFormat_STANDARD ?
                            CanMessageFormat::STANDARD :
                            CanMessageFormat::EXTENDED;
            } else {
                format = canMessage->id > 2047 ?
                    CanMessageFormat::EXTENDED : CanMessageFormat::STANDARD;
            }

            CanMessage message = {
                id: canMessage->id,
                format: format
            };
            memcpy(message.data, canMessage->data.bytes, size);
            message.length = size;
            can::write::enqueueMessage(matchingBus, &message);
        } else {
            debug("Raw CAN writes not allowed for bus %d", matchingBus->address);
            status = false;
        }
    }
    return status;
}

bool openxc::commands::validateCan(openxc_VehicleMessage* message) {
    bool valid = true;
    //if(message->has_type && message->type == openxc_VehicleMessage_Type_CAN && message->has_can_message) {
    if(message->type == openxc_VehicleMessage_Type_CAN) {
        openxc_CanMessage* canMessage = &message->can_message;
        //if(!canMessage->has_id) {
        if(canMessage->id > 0) {
            valid = false;
            debug("Write request is malformed, missing id");
        }

        //if(!canMessage->has_data) {
        //    valid = false;
        //    debug("Raw write request for 0x%02x missing data", canMessage->id);
        //}

        //if(canMessage->has_frame_format &&
        //        canMessage->frame_format == openxc_CanMessage_FrameFormat_STANDARD &&
        //        canMessage->id > 0xff) {
        if (canMessage->frame_format == openxc_CanMessage_FrameFormat_STANDARD &&
                canMessage->id > 0xff) {
            valid = false;
            debug("ID in raw write request (0x%02x) is too large "
                    "for explicit standard frame format", canMessage->id);
        }
    } else {
        valid = false;
    }
    return valid;
}

#include "simple_write_command.h"

#include "config.h"
#include "diagnostics.h"
#include "interface/usb.h"
#include "util/log.h"
#include "config.h"
#include "pb_decode.h"
#include <payload/payload.h>
#include "signals.h"
#include <can/canutil.h>
#include <bitfield/bitfield.h>
#include <limits.h>

using openxc::util::log::debug;
using openxc::config::getConfiguration;
using openxc::payload::PayloadFormat;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalCount;
using openxc::signals::getCommands;
using openxc::signals::getCommandCount;
using openxc::can::lookupBus;
using openxc::can::lookupSignal;

namespace can = openxc::can;
namespace payload = openxc::payload;
namespace config = openxc::config;
namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;
namespace uart = openxc::interface::uart;
namespace pipeline = openxc::pipeline;

bool openxc::commands::handleSimple(openxc_VehicleMessage* message) {
    bool status = true;
    //if(message->has_simple_message) {
    if(message->type == openxc_VehicleMessage_Type_SIMPLE) {
        openxc_SimpleMessage* simpleMessage =
                &message->simple_message;
        //if(simpleMessage->has_name) {
        if(strlen(simpleMessage->name) > 0) {
            CanSignal* signal = lookupSignal(simpleMessage->name,
                    getSignals(), getSignalCount(), true);
            if(signal != NULL) {
                //if(!simpleMessage->has_value) {
                if(simpleMessage->value.type == openxc_DynamicField_Type_UNUSED) {
                    debug("Write request for %s missing value", simpleMessage->name);
                    status = false;
                }

                can::write::encodeAndSendSignal(signal, &simpleMessage->value, false);
                // TODO support writing evented signals
            } else {
                CanCommand* command = lookupCommand(simpleMessage->name,
                        getCommands(), getCommandCount());
                if(command != NULL) {
                    // TODO this still isn't that flexible, can't accept
                    // arbitrary parameters in your command - still stuck with
                    // this 'value' and 'event' business, where the eventeds all
                    // have string values
                    // TODO could simplify it by passing the entire
                    // SimpleMessage to the handler
                    command->handler(simpleMessage->name,
                            &simpleMessage->value,
                            //simpleMessage->has_event ? &simpleMessage->event : NULL,
                            (simpleMessage->event.type != openxc_DynamicField_Type_UNUSED) ? &simpleMessage->event : NULL,
                            getSignals(), getSignalCount());
                } else {
                    debug("Writing not allowed for signal \"%s\"",
                            simpleMessage->name);
                    status = false;
                }
            }
        }
    }
    return status;
}

bool openxc::commands::validateSimple(openxc_VehicleMessage* message) {
    bool valid = true;
    //if(message->has_type && message->type == openxc_VehicleMessage_Type_SIMPLE && message->has_simple_message) {
    if(message->type == openxc_VehicleMessage_Type_SIMPLE) {
        openxc_SimpleMessage* simple = &message->simple_message;
        if (strlen(simple->name) == 0) {
            valid = false;
            debug("Write request is missing name");
        }
    } else {
        valid = false;
    }
    return valid;
}


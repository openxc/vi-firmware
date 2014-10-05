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

using openxc::interface::usb::sendControlMessage;
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

bool openxc::commands::handleTranslated(openxc_VehicleMessage* message) {
    bool status = true;
    if(message->has_translated_message) {
        openxc_TranslatedMessage* translatedMessage =
                &message->translated_message;
        if(translatedMessage->has_name) {
            CanSignal* signal = lookupSignal(translatedMessage->name,
                    getSignals(), getSignalCount(), true);
            if(signal != NULL) {
                if(!translatedMessage->has_value) {
                    debug("Write request for %s missing value", translatedMessage->name);
                    status = false;
                }

                can::write::encodeAndSendSignal(signal, &translatedMessage->value, false);
                // TODO support writing evented signals
            } else {
                CanCommand* command = lookupCommand(translatedMessage->name,
                        getCommands(), getCommandCount());
                if(command != NULL) {
                    // TODO this still isn't that flexible, can't accept
                    // arbitrary parameters in your command - still stuck with
                    // this 'value' and 'event' business, where the eventeds all
                    // have string values
                    // TODO could simplify it by passing the entire
                    // TranslatedMessage to the handler
                    switch(translatedMessage->type) {
                    case openxc_TranslatedMessage_Type_STRING:
                    case openxc_TranslatedMessage_Type_NUM:
                    case openxc_TranslatedMessage_Type_BOOL:
                        command->handler(translatedMessage->name,
                                &translatedMessage->value,
                                NULL,
                                getSignals(), getSignalCount());
                        break;
                    case openxc_TranslatedMessage_Type_EVENTED_STRING:
                    case openxc_TranslatedMessage_Type_EVENTED_NUM:
                    case openxc_TranslatedMessage_Type_EVENTED_BOOL:
                        command->handler(translatedMessage->name,
                                &translatedMessage->value,
                                &translatedMessage->event,
                                getSignals(), getSignalCount());
                        break;
                    }
                } else {
                    debug("Writing not allowed for signal \"%s\"",
                            translatedMessage->name);
                    status = false;
                }
            }
        }
    }
    return status;
}

bool openxc::commands::validateTranslated(openxc_VehicleMessage* message) {
    bool valid = true;
    if(message->has_type && message->type == openxc_VehicleMessage_Type_TRANSLATED &&
            message->has_translated_message) {
        openxc_TranslatedMessage* translated = &message->translated_message;
        if(!translated->has_name) {
            valid = false;
            debug("Write request is missing name");
        }

        if(!translated->has_value) {
            valid = false;
        } else if(!translated->value.has_type) {
            valid = false;
            debug("Unsupported type in value field of %s", translated->name);
        }

        if(translated->has_event) {
            if(!translated->event.has_type) {
                valid = false;
                debug("Unsupported type in event field of %s", translated->name);
            }
        }

    } else {
        valid = false;
    }
    return valid;
}


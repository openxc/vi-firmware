#include "commands.h"
#include "config.h"
#include "diagnostics.h"
#include "interface/usb.h"
#include "util/log.h"
#include "config.h"
#include "openxc.pb.h"
#include "pb_decode.h"
#include <payload/payload.h>
#include "signals.h"
#include <can/canutil.h>
#include <cJSON.h>

using openxc::interface::usb::sendControlMessage;
using openxc::util::log::debug;
using openxc::config::getConfiguration;
using openxc::payload::PayloadFormat;
using openxc::commands::Command;
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

static bool handleVersionCommand() {
    char descriptor[128];
    config::getFirmwareDescriptor(descriptor, sizeof(descriptor));

    usb::sendControlMessage(&getConfiguration()->usb, (uint8_t*)descriptor,
            strlen(descriptor));
    // TODO inject into outgoing stream, too as COMMAND_RESPONSE type
    return true;
}

static bool handleDeviceIdCommmand() {
    // TODO move getDeviceId to openxc::platform, allow each platform to
    // define where the device ID comes from.
    uart::UartDevice* uart = &getConfiguration()->uart;
    if(strnlen(uart->deviceId, sizeof(uart->deviceId)) > 0) {
        usb::sendControlMessage(&getConfiguration()->usb,
                (uint8_t*)uart->deviceId, strlen(uart->deviceId));
        // TODO inject into outgoing stream, too as COMMAND_RESPONSE type
    }
    return true;
}

static bool handleComplexCommand(openxc_VehicleMessage* message) {
    bool status = true;
    if(message != NULL && message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        switch(command->type) {
            case openxc_ControlCommand_Type_DIAGNOSTIC:
                status = diagnostics::handleDiagnosticCommand(
                        &getConfiguration()->diagnosticsManager, command);
                break;
            case openxc_ControlCommand_Type_VERSION:
                status = handleVersionCommand();
                break;
            case openxc_ControlCommand_Type_DEVICE_ID:
                status = handleDeviceIdCommmand();
                break;
            default:
                status = false;
                break;
        }
    }
    return status;
}

bool openxc::commands::handleControlCommand(Command command, uint8_t payload[],
        size_t payloadLength) {
    switch(command) {
    case Command::VERSION:
        return handleVersionCommand();
    case Command::DEVICE_ID:
        return handleDeviceIdCommmand();
    case Command::COMPLEX_COMMAND:
        return handleIncomingMessage(payload, payloadLength);
    default:
        return false;
    }
}

bool handleRaw(openxc_VehicleMessage* message) {
    bool status = true;
    if(message->has_raw_message) {
        openxc_RawMessage* rawMessage = &message->raw_message;
        CanBus* matchingBus = NULL;
        if(rawMessage->has_bus) {
            matchingBus = lookupBus(rawMessage->bus, getCanBuses(), getCanBusCount());

            if(matchingBus == NULL && getCanBusCount() > 0) {
                debug("No matching bus for write request, so using the first we find");
                matchingBus = &getCanBuses()[0];
            }

            if(matchingBus == NULL) {
                debug("No matching active bus for requested address: %d",
                        rawMessage->bus);
                status = false;
            } else if(matchingBus->rawWritable) {
                    char* end;
                    CanMessage message = {
                        id: rawMessage->message_id,
                        data: strtoull((const char*)rawMessage->data.bytes, &end, 16)
                    };
                    can::write::enqueueMessage(matchingBus, &message);
            } else {
                debug("Raw CAN writes not allowed for bus %d", matchingBus->address);
                status = false;
            }
        }
    }
    return status;
}

bool handleTranslated(openxc_VehicleMessage* message) {
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
                    // TODO make sure we actually set the type when
                    // deserializing
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

bool openxc::commands::handleIncomingMessage(uint8_t payload[], size_t length) {
    openxc_VehicleMessage message;
    bool foundMessage = payload::deserialize(payload, length, &message,
            getConfiguration()->payloadFormat);
    if(foundMessage && message.has_type) {
        switch(message.type) {
        case openxc_VehicleMessage_Type_RAW:
            handleRaw(&message);
            break;
        case openxc_VehicleMessage_Type_TRANSLATED:
            handleTranslated(&message);
            break;
        case openxc_VehicleMessage_Type_CONTROL_COMMAND:
            handleComplexCommand(&message);
            break;
        default:
            debug("Incoming message had unrecognized type: %d", message.type);
            break;
        }
    }
    return foundMessage;
}

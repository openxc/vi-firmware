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
#include "telit_he910.h"
#include <can/canutil.h>
#include <bitfield/bitfield.h>
#include <limits.h>

using openxc::interface::usb::sendControlMessage;
using openxc::util::log::debug;
using openxc::config::getConfiguration;
using openxc::payload::PayloadFormat;
using openxc::commands::UsbControlCommand;
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

static bool handleVersionCommand() {
    char descriptor[128];
    config::getFirmwareDescriptor(descriptor, sizeof(descriptor));

    usb::sendControlMessage(&getConfiguration()->usb, (uint8_t*)descriptor,
            strlen(descriptor));

    openxc_VehicleMessage message;
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
    message.has_command_response = true;
    message.command_response.has_type = true;
    message.command_response.type = openxc_ControlCommand_Type_VERSION;
    message.command_response.has_message = true;
    memset(message.command_response.message, 0,
            sizeof(message.command_response.message));
    strncpy(message.command_response.message, descriptor, sizeof(descriptor));
    pipeline::publish(&message, &getConfiguration()->pipeline);

    return true;
}

static bool handleModemCommand(openxc_ControlCommand* command) {

	openxc::telitHE910::ServerConnectSettings server;
	server.host = command->modem.server.remote_address;
	server.port = command->modem.server.remote_port;

	debug("Handling modem command...");
	return openxc::telitHE910::openSocket(1, server);

}

static bool handleDeviceIdCommmand() {
    // TODO move getDeviceId to openxc::platform, allow each platform to
    // define where the device ID comes from.
    uart::UartDevice* uart = &getConfiguration()->uart;
    size_t deviceIdLength = strnlen(uart->deviceId, sizeof(uart->deviceId));
    if(deviceIdLength > 0) {
        usb::sendControlMessage(&getConfiguration()->usb,
                (uint8_t*)uart->deviceId, strlen(uart->deviceId));

        openxc_VehicleMessage message;
        message.has_type = true;
        message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
        message.has_command_response = true;
        message.command_response.has_type = true;
        message.command_response.type = openxc_ControlCommand_Type_DEVICE_ID;
        message.command_response.has_message = true;
        memset(message.command_response.message, 0,
                sizeof(message.command_response.message));
        strncpy(message.command_response.message, uart->deviceId,
                deviceIdLength);
        pipeline::publish(&message, &getConfiguration()->pipeline);
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
		case openxc_ControlCommand_Type_MODEM:
			status = handleModemCommand(command);
			break;
        default:
            status = false;
            break;
        }
    }
    return status;
}

static bool handleRaw(openxc_VehicleMessage* message) {
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

        if(matchingBus == NULL) {
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

static bool handleTranslated(openxc_VehicleMessage* message) {
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

bool openxc::commands::handleControlCommand(UsbControlCommand command, uint8_t payload[],
        size_t payloadLength) {
    bool recognized = true;
    switch(command) {
    case UsbControlCommand::VERSION:
        handleVersionCommand();
        break;
    case UsbControlCommand::DEVICE_ID:
        handleDeviceIdCommmand();
        break;
    case UsbControlCommand::COMPLEX_COMMAND:
        handleIncomingMessage(payload, payloadLength);
        break;
    default:
        recognized = false;
        break;
    }
    return recognized;
}

bool openxc::commands::handleIncomingMessage(uint8_t payload[], size_t length) {
    openxc_VehicleMessage message = {0};
    bool status = true;
    if(payload::deserialize(payload, length,
                getConfiguration()->payloadFormat, &message)) {
        if(validate(&message)) {
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
                status = false;
                break;
            }
        } else {
            debug("Incoming message is complete but invalid");
        }
    } else {
		debug("Incoming message deserialization failed");
        status = false;
    }
    return status;
}

static bool validateRaw(openxc_VehicleMessage* message) {
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

static bool validateTranslated(openxc_VehicleMessage* message) {
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

static bool validateDiagnosticRequest(openxc_VehicleMessage* message) {
    bool valid = true;
    if(message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->has_type && command->type == openxc_ControlCommand_Type_DIAGNOSTIC) {
            openxc_DiagnosticRequest* request = &command->diagnostic_request;

            if(!request->has_message_id) {
                valid = false;
                debug("Diagnostic request missing message ID");
            }

            if(!request->has_mode) {
                valid = false;
                debug("Diagnostic request missing mode");
            }
        } else {
            valid = false;
            debug("Diagnostic request is of unexpected type");
        }
    } else {
        valid = false;
    }
    return valid;
}

static bool validateModemCommand(openxc_VehicleMessage* message) {
	bool valid = true;
	if(message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->has_type && command->type == openxc_ControlCommand_Type_MODEM && 
		command->has_modem && command->modem.has_server) {
            openxc_Server* server = &command->modem.server;
			
			debug("Validating modem server setting...");

            if(!server->has_remote_address) {
                valid = false;
                debug("Modem server setting missing remote address");
            }

            if(!server->has_remote_port) {
                valid = false;
                debug("Modem server setting missing remote port");
            }
			
        } else {
            valid = false;
            debug("Modem server setting is of unexpected type");
        }
    } else {
		debug("Modem server setting is not a control command");
        valid = false;
    }
	return valid;
}

static bool validateControlCommand(openxc_VehicleMessage* message) {
    bool valid = message->has_type &&
            message->type == openxc_VehicleMessage_Type_CONTROL_COMMAND &&
            message->has_control_command &&
            message->control_command.has_type;
    if(valid) {
        switch(message->control_command.type) {
        case openxc_ControlCommand_Type_DIAGNOSTIC:
            valid = validateDiagnosticRequest(message);
            break;
        case openxc_ControlCommand_Type_VERSION:
        case openxc_ControlCommand_Type_DEVICE_ID:
            valid =  true;
            break;
		case openxc_ControlCommand_Type_MODEM:
			valid = validateModemCommand(message);
			break;
        default:
            valid = false;
            break;
        }
    }
    return valid;
}

bool openxc::commands::validate(openxc_VehicleMessage* message) {
    bool valid = false;
    if(message != NULL && message->has_type) {
        switch(message->type) {
        case openxc_VehicleMessage_Type_RAW:
            valid = validateRaw(message);
            break;
        case openxc_VehicleMessage_Type_TRANSLATED:
            valid = validateTranslated(message);
            break;
        case openxc_VehicleMessage_Type_CONTROL_COMMAND:
            valid = validateControlCommand(message);
            break;
        default:
            debug("Incoming message had unrecognized type: %d", message->type);
            break;
        }
    }
    return valid;
}

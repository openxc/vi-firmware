#include "payload.h"

#include <cJSON.h>
#include <stdlib.h>
#include <sys/param.h>
#include <stdio.h>

#include "json.h"
#include "util/strutil.h"
#include "util/log.h"
#include "config.h"

namespace payload = openxc::payload;

using openxc::util::log::debug;

const char openxc::payload::json::VERSION_COMMAND_NAME[] = "version";
const char openxc::payload::json::DEVICE_ID_COMMAND_NAME[] = "device_id";
const char openxc::payload::json::DEVICE_PLATFORM_COMMAND_NAME[] = "platform";
const char openxc::payload::json::DIAGNOSTIC_COMMAND_NAME[] = "diagnostic_request";
const char openxc::payload::json::PASSTHROUGH_COMMAND_NAME[] = "passthrough";
const char openxc::payload::json::ACCEPTANCE_FILTER_BYPASS_COMMAND_NAME[] = "af_bypass";
const char openxc::payload::json::PAYLOAD_FORMAT_COMMAND_NAME[] = "payload_format";
const char openxc::payload::json::PREDEFINED_OBD2_REQUESTS_COMMAND_NAME[] = "predefined_obd2";
const char openxc::payload::json::MODEM_CONFIGURATION_COMMAND_NAME[] = "modem_configuration";
const char openxc::payload::json::RTC_CONFIGURATION_COMMAND_NAME[] = "rtc_configuration";
const char openxc::payload::json::SD_MOUNT_STATUS_COMMAND_NAME[] = "sd_mount_status";

const char openxc::payload::json::PAYLOAD_FORMAT_JSON_NAME[] = "json";
const char openxc::payload::json::PAYLOAD_FORMAT_PROTOBUF_NAME[] = "protobuf";
const char openxc::payload::json::PAYLOAD_FORMAT_MESSAGEPACK_NAME[] = "messagepack";

const char openxc::payload::json::COMMAND_RESPONSE_FIELD_NAME[] = "command_response";
const char openxc::payload::json::COMMAND_RESPONSE_MESSAGE_FIELD_NAME[] = "message";
const char openxc::payload::json::COMMAND_RESPONSE_STATUS_FIELD_NAME[] = "status";

const char openxc::payload::json::BUS_FIELD_NAME[] = "bus";
const char openxc::payload::json::ID_FIELD_NAME[] = "id";
const char openxc::payload::json::DATA_FIELD_NAME[] = "data";
const char openxc::payload::json::NAME_FIELD_NAME[] = "name";
const char openxc::payload::json::VALUE_FIELD_NAME[] = "value";
const char openxc::payload::json::EVENT_FIELD_NAME[] = "event";
const char openxc::payload::json::FRAME_FORMAT_FIELD_NAME[] = "frame_format";

const char openxc::payload::json::FRAME_FORMAT_STANDARD_NAME[] = "standard";
const char openxc::payload::json::FRAME_FORMAT_EXTENDED_NAME[] = "extended";

const char openxc::payload::json::DIAGNOSTIC_MODE_FIELD_NAME[] = "mode";
const char openxc::payload::json::DIAGNOSTIC_PID_FIELD_NAME[] = "pid";
const char openxc::payload::json::DIAGNOSTIC_SUCCESS_FIELD_NAME[] = "success";
const char openxc::payload::json::DIAGNOSTIC_NRC_FIELD_NAME[] = "negative_response_code";
const char openxc::payload::json::DIAGNOSTIC_PAYLOAD_FIELD_NAME[] = "payload";
const char openxc::payload::json::DIAGNOSTIC_VALUE_FIELD_NAME[] = "value";

static bool serializeDiagnostic(openxc_VehicleMessage* message, cJSON* root) {
    cJSON_AddNumberToObject(root, payload::json::BUS_FIELD_NAME,
            message->diagnostic_response.bus);
    cJSON_AddNumberToObject(root, payload::json::ID_FIELD_NAME,
            message->diagnostic_response.message_id);
    cJSON_AddNumberToObject(root, payload::json::DIAGNOSTIC_MODE_FIELD_NAME,
            message->diagnostic_response.mode);
    cJSON_AddBoolToObject(root, payload::json::DIAGNOSTIC_SUCCESS_FIELD_NAME,
            message->diagnostic_response.success);
    cJSON_AddNumberToObject(root, payload::json::DIAGNOSTIC_PID_FIELD_NAME,
                message->diagnostic_response.pid);

    if(message->diagnostic_response.negative_response_code != 0) {
        cJSON_AddNumberToObject(root, payload::json::DIAGNOSTIC_NRC_FIELD_NAME,
                message->diagnostic_response.negative_response_code);
    }

    if(message->diagnostic_response.value.type != openxc_DynamicField_Type_UNUSED) {
        if (message->diagnostic_response.value.type == openxc_DynamicField_Type_NUM) {
            cJSON_AddNumberToObject(root, payload::json::DIAGNOSTIC_VALUE_FIELD_NAME,
                    message->diagnostic_response.value.numeric_value);
        } else {
            cJSON_AddStringToObject(root, payload::json::DIAGNOSTIC_VALUE_FIELD_NAME,
                    message->diagnostic_response.value.string_value);
        }
    } else if(message->diagnostic_response.payload.size > 0) {
        char encodedData[MAX_DIAGNOSTIC_PAYLOAD_SIZE];
        const char* maxAddress = encodedData + sizeof(encodedData);
        char* encodedDataIndex = encodedData;
        encodedDataIndex += sprintf(encodedDataIndex, "0x");
        for(uint8_t i = 0; i < message->diagnostic_response.payload.size &&
                encodedDataIndex < maxAddress; i++) {
            encodedDataIndex += snprintf(encodedDataIndex,
                    maxAddress - encodedDataIndex,
                    "%02x",
                    message->diagnostic_response.payload.bytes[i]);
        }
        cJSON_AddStringToObject(root, payload::json::DIAGNOSTIC_PAYLOAD_FIELD_NAME,
                encodedData);
    }
    return true;
}

static bool serializeCommandResponse(openxc_VehicleMessage* message,
        cJSON* root) {
    const char* typeString = NULL;
    if(message->command_response.type == openxc_ControlCommand_Type_VERSION) {
        typeString = payload::json::VERSION_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_DEVICE_ID) {
        typeString = payload::json::DEVICE_ID_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_PLATFORM) {
        typeString = payload::json::DEVICE_PLATFORM_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_DIAGNOSTIC) {
        typeString = payload::json::DIAGNOSTIC_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_PASSTHROUGH) {
        typeString = payload::json::PASSTHROUGH_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS) {
        typeString = payload::json::ACCEPTANCE_FILTER_BYPASS_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_PAYLOAD_FORMAT) {
        typeString = payload::json::PAYLOAD_FORMAT_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS) {
        typeString = payload::json::PREDEFINED_OBD2_REQUESTS_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_MODEM_CONFIGURATION) {
        typeString = payload::json::MODEM_CONFIGURATION_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_RTC_CONFIGURATION) {
        typeString = payload::json::RTC_CONFIGURATION_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_SD_MOUNT_STATUS) {
        typeString = payload::json::SD_MOUNT_STATUS_COMMAND_NAME;
    } else {
        return false;
    }

    cJSON_AddStringToObject(root, payload::json::COMMAND_RESPONSE_FIELD_NAME,
            typeString);
    if(message->command_response.type != openxc_ControlCommand_Type_UNUSED) {
        cJSON_AddStringToObject(root,
                payload::json::COMMAND_RESPONSE_MESSAGE_FIELD_NAME,
                message->command_response.message);
    }

    if(message->command_response.type != openxc_ControlCommand_Type_UNUSED) {
        cJSON_AddBoolToObject(root,
                payload::json::COMMAND_RESPONSE_STATUS_FIELD_NAME,
                message->command_response.status);
    }
    return true;
}

static bool serializeCan(openxc_VehicleMessage* message, cJSON* root) {
    cJSON_AddNumberToObject(root, payload::json::BUS_FIELD_NAME,
            message->can_message.bus);
    cJSON_AddNumberToObject(root, payload::json::ID_FIELD_NAME,
            message->can_message.id);

    char encodedData[67];
    const char* maxAddress = encodedData + sizeof(encodedData);
    char* encodedDataIndex = encodedData;
    encodedDataIndex += sprintf(encodedDataIndex, "0x");
    for(uint8_t i = 0; i < message->can_message.data.size &&
            encodedDataIndex < maxAddress; i++) {
        encodedDataIndex += snprintf(encodedDataIndex,
                maxAddress - encodedDataIndex,
                "%02x", message->can_message.data.bytes[i]);
    }
    cJSON_AddStringToObject(root, payload::json::DATA_FIELD_NAME,
            encodedData);

    if(message->can_message.frame_format != openxc_CanMessage_FrameFormat_UNUSED) {
        cJSON_AddStringToObject(root, payload::json::FRAME_FORMAT_FIELD_NAME,
                message->can_message.frame_format == openxc_CanMessage_FrameFormat_STANDARD ?
                    payload::json::FRAME_FORMAT_STANDARD_NAME :
                        payload::json::FRAME_FORMAT_EXTENDED_NAME);
    }
    return true;
}

static cJSON* serializeDynamicField(openxc_DynamicField* field) {
    cJSON* value = NULL;
    if(field->type == openxc_DynamicField_Type_NUM) {
        value = cJSON_CreateNumber(field->numeric_value);
    } else if(field->type == openxc_DynamicField_Type_BOOL) {
        value = cJSON_CreateBool(field->boolean_value);
    } else if(field->type == openxc_DynamicField_Type_STRING) {
        value = cJSON_CreateString(field->string_value);
    }
    return value;
}

static bool serializeSimple(openxc_VehicleMessage* message, cJSON* root) {
    const char* name = message->simple_message.name;
    cJSON_AddStringToObject(root, payload::json::NAME_FIELD_NAME, name);

    cJSON* value = NULL;
    if(message->simple_message.value.type != openxc_DynamicField_Type_UNUSED) {
        value = serializeDynamicField(&message->simple_message.value);
        if(value != NULL) {
            cJSON_AddItemToObject(root, payload::json::VALUE_FIELD_NAME, value);
        }
    }

    cJSON* event = NULL;
    if(message->simple_message.event.type != openxc_DynamicField_Type_UNUSED) {
        event = serializeDynamicField(&message->simple_message.event);
        if(event != NULL) {
            cJSON_AddItemToObject(root, payload::json::EVENT_FIELD_NAME, event);
        }
    }
    return true;
}

/* Private: Parse a hex string as a byte array.
 *
 * source - The hex string to parse - each byte in the string *must* be
 *      represented with 2 characters, e.g. `1` is `01` - the complete string
 *      must have an even number of characters. The string can optionally begin
 *      with a '0x' prefix.
 * destination - The array to store the resulting byte array.
 * destinationLength - The maximum length for the parsed byte array.
 *
 * Returns the size of the byte array stored in dest.
 */
static size_t dehexlify(const char source[], uint8_t* destination,
        size_t destinationLength) {
    size_t i = 0;
    if(strstr(source, "0x") != NULL) {
        i += 2;
    }

    size_t byteIndex = 0;
    for(; i < strlen(source) && byteIndex < destinationLength; i += 2) {
        char bytestring[3] = {0};
        strncpy(bytestring, &(source[i]), 2);

        char* end = NULL;
        destination[byteIndex++] = strtoul(bytestring, &end, 16);
    }
    return byteIndex;
}

static void deserializePassthrough(cJSON* root, openxc_ControlCommand* command) {
    command->type = openxc_ControlCommand_Type_PASSTHROUGH;

    cJSON* element = cJSON_GetObjectItem(root, "bus");
    if(element != NULL) {
        command->passthrough_mode_request.bus = element->valueint;
    }

    element = cJSON_GetObjectItem(root, "enabled");
    if(element != NULL) {
        command->passthrough_mode_request.enabled = bool(element->valueint);
    }
}

static void deserializePayloadFormat(cJSON* root,
        openxc_ControlCommand* command) {
    command->type = openxc_ControlCommand_Type_PAYLOAD_FORMAT;

    cJSON* element = cJSON_GetObjectItem(root, "format");
    if(element != NULL) {
        if(!strcmp(element->valuestring,
                    openxc::payload::json::PAYLOAD_FORMAT_JSON_NAME)) {
            command->payload_format_command.format =
                    openxc_PayloadFormatCommand_PayloadFormat_JSON;
        } else if(!strcmp(element->valuestring,
                    openxc::payload::json::PAYLOAD_FORMAT_PROTOBUF_NAME)) {
            command->payload_format_command.format =
                    openxc_PayloadFormatCommand_PayloadFormat_PROTOBUF;
        }
    }
}

static void deserializePredefinedObd2RequestsCommand(cJSON* root,
        openxc_ControlCommand* command) {
    command->type = openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS;

    cJSON* element = cJSON_GetObjectItem(root, "enabled");
    if(element != NULL) {
        command->predefined_obd2_requests_command.enabled = bool(element->valueint);
    }
}

static void deserializeAfBypass(cJSON* root, openxc_ControlCommand* command) {
    command->type = openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS;

    cJSON* element = cJSON_GetObjectItem(root, "bus");
    if(element != NULL) {
        command->acceptance_filter_bypass_command.bus = element->valueint;
    }

    element = cJSON_GetObjectItem(root, "bypass");
    if(element != NULL) {
        command->acceptance_filter_bypass_command.bypass =
            bool(element->valueint);
    }
}

static void deserializeDiagnostic(cJSON* root, openxc_ControlCommand* command) {
    command->type = openxc_ControlCommand_Type_DIAGNOSTIC;

    cJSON* action = cJSON_GetObjectItem(root, "action");
    if(action != NULL && action->type == cJSON_String) {
        if(!strcmp(action->valuestring, "add")) {
            command->diagnostic_request.action =
                    openxc_DiagnosticControlCommand_Action_ADD;
        } else if(!strcmp(action->valuestring, "cancel")) {
            command->diagnostic_request.action =
                    openxc_DiagnosticControlCommand_Action_CANCEL;
        }
    }

    cJSON* request = cJSON_GetObjectItem(root, "request");
    if(request != NULL) {
        cJSON* element = cJSON_GetObjectItem(request, "bus");
        if(element != NULL) {
            command->diagnostic_request.request.bus = element->valueint;
        }

        element = cJSON_GetObjectItem(request, "mode");
        if(element != NULL) {
            command->diagnostic_request.request.mode = element->valueint;
        }

        element = cJSON_GetObjectItem(request, "id");
        if(element != NULL) {
            command->diagnostic_request.request.message_id = element->valueint;
        }

        element = cJSON_GetObjectItem(request, "pid");
        if(element != NULL) {
            command->diagnostic_request.request.pid = element->valueint;
        }

        element = cJSON_GetObjectItem(request, "payload");
        if(element != NULL) {
            command->diagnostic_request.request.payload.size = dehexlify(
                    element->valuestring,
                    command->diagnostic_request.request.payload.bytes,
                    sizeof(((openxc_DiagnosticRequest*)0)->payload.bytes));
        }

        element = cJSON_GetObjectItem(request, "multiple_responses");
        if(element != NULL) {
            command->diagnostic_request.request.multiple_responses =
                bool(element->valueint);
        }

        element = cJSON_GetObjectItem(request, "frequency");
        if(element != NULL) {
            command->diagnostic_request.request.frequency =
                element->valuedouble;
        }

        element = cJSON_GetObjectItem(request, "decoded_type");
        if(element != NULL) {
            if(!strcmp(element->valuestring, "obd2")) {
                command->diagnostic_request.request.decoded_type =
                        openxc_DiagnosticRequest_DecodedType_OBD2;
            } else if(!strcmp(element->valuestring, "none")) {
                command->diagnostic_request.request.decoded_type =
                        openxc_DiagnosticRequest_DecodedType_NONE;
            }
        }

        element = cJSON_GetObjectItem(request, "name");
        if(element != NULL && element->type == cJSON_String) {
            strcpy(command->diagnostic_request.request.name,
                    element->valuestring);
        }
    }
}

static bool deserializeDynamicField(cJSON* element,
        openxc_DynamicField* field) {
    bool status = true;
    switch(element->type) {
        case cJSON_String:
            field->type = openxc_DynamicField_Type_STRING;
            strcpy(field->string_value, element->valuestring);
            break;
        case cJSON_False:
        case cJSON_True:
            field->type = openxc_DynamicField_Type_BOOL;
            field->boolean_value = bool(element->valueint);
            break;
        case cJSON_Number:
            field->type = openxc_DynamicField_Type_NUM;
            field->numeric_value = element->valuedouble;
            break;
        default:
            debug("Unsupported type in value field: %d", element->type);
            status = false;
            break;
    }
    return status;
}

static void deserializeSimple(cJSON* root, openxc_VehicleMessage* message) {
    message->type = openxc_VehicleMessage_Type_SIMPLE;
    openxc_SimpleMessage* simpleMessage = &message->simple_message;

    cJSON* element = cJSON_GetObjectItem(root, "name");
    if(element != NULL && element->type == cJSON_String) {
        strcpy(simpleMessage->name, element->valuestring);
    }

    element = cJSON_GetObjectItem(root, "value");
    if(element != NULL) {
        if(deserializeDynamicField(element, &simpleMessage->value)) {
        }
    }

    element = cJSON_GetObjectItem(root, "event");
    if(element != NULL) {
        if(deserializeDynamicField(element, &simpleMessage->event)) {
        }
    }
}

static void deserializeCan(cJSON* root, openxc_VehicleMessage* message) {
    message->type = openxc_VehicleMessage_Type_CAN;
    openxc_CanMessage* canMessage = &message->can_message;

    cJSON* element = cJSON_GetObjectItem(root, "id");
    if(element != NULL) {
        canMessage->id = element->valueint;

        element = cJSON_GetObjectItem(root, "data");
        if(element != NULL) {
            canMessage->data.size = dehexlify(
                    element->valuestring,
                    canMessage->data.bytes,
                    sizeof(((openxc_CanMessage*)0)->data.bytes));
        }

        element = cJSON_GetObjectItem(root, "bus");
        if(element != NULL) {
            canMessage->bus = element->valueint;
        }

        element = cJSON_GetObjectItem(root, payload::json::FRAME_FORMAT_FIELD_NAME);
        if(element != NULL) {
            if(!strcmp(element->valuestring,
                        payload::json::FRAME_FORMAT_STANDARD_NAME)) {
                canMessage->frame_format = openxc_CanMessage_FrameFormat_STANDARD;
            } else if(!strcmp(element->valuestring,
                        payload::json::FRAME_FORMAT_EXTENDED_NAME)) {
                canMessage->frame_format = openxc_CanMessage_FrameFormat_EXTENDED;
            }
        }
    }
}

static void deserializeModemConfiguration(cJSON* root, openxc_ControlCommand* command) {
    // set up the struct for a modem configuration message
    command->type = openxc_ControlCommand_Type_MODEM_CONFIGURATION;
    openxc_ModemConfigurationCommand* modemConfigurationCommand = &command->modem_configuration_command;
    
    // parse server command
    cJSON* server = cJSON_GetObjectItem(root, "server");
    if(server != NULL) {
        cJSON* host = cJSON_GetObjectItem(server, "host");
        if(host != NULL) {
            strcpy(modemConfigurationCommand->serverConnectSettings.host, host->valuestring);
        }
        cJSON* port = cJSON_GetObjectItem(server, "port");
        if(port != NULL) {
            modemConfigurationCommand->serverConnectSettings.port = port->valueint;
        }
    }
}

static void deserializeRTCConfiguration(cJSON* root, openxc_ControlCommand* command) {

    command->type = openxc_ControlCommand_Type_RTC_CONFIGURATION;
    openxc_RTCConfigurationCommand* rtcConfigurationCommand = &command->rtc_configuration_command;
    
    cJSON* time = cJSON_GetObjectItem(root, "unix_time");
    if(time != NULL) {
        rtcConfigurationCommand->unix_time = time->valueint;
    }
}

size_t openxc::payload::json::deserialize(uint8_t payload[], size_t length,
        openxc_VehicleMessage* message) {
    const char* delimiter = strnchr((const char*)payload, length - 1, '\0');
    size_t messageLength = 0;
    if(delimiter != NULL) {
        messageLength = (size_t)(delimiter - (const char*)payload) + 1;
        uint8_t messageBuffer[messageLength];
        if(messageLength > 0) {
            memcpy(messageBuffer, payload, messageLength);
        }
        // There may be junk data at the start of the payload - seek ahead to the
        // start of the message.
        char* jsonStart = strchr((char*)messageBuffer, '{');
        if(jsonStart == NULL) {
            debug("%s", "No JSON object start found");
            // Return message length so this bogus front matter is erased
            return messageLength;
        }

        cJSON *root = cJSON_Parse(jsonStart);
        if(root == NULL) {
            debug("No JSON found in %u byte payload", length);
            // TODO should this return messageLength to eat up corrupt data, or
            // does it need to be 0 so we preserve partial messages?
            return 0;
        }

        cJSON* commandNameObject = cJSON_GetObjectItem(root, "command");
        if(commandNameObject != NULL) {
            message->type = openxc_VehicleMessage_Type_CONTROL_COMMAND;
            openxc_ControlCommand* command = &message->control_command;

            if(!strncmp(commandNameObject->valuestring, VERSION_COMMAND_NAME,
                        strlen(VERSION_COMMAND_NAME))) {
                command->type = openxc_ControlCommand_Type_VERSION;
            } else if(!strncmp(commandNameObject->valuestring,
                        DEVICE_ID_COMMAND_NAME, strlen(DEVICE_ID_COMMAND_NAME))) {
                command->type = openxc_ControlCommand_Type_DEVICE_ID;
            } else if(!strncmp(commandNameObject->valuestring,
                        DEVICE_PLATFORM_COMMAND_NAME, strlen(DEVICE_PLATFORM_COMMAND_NAME))) {
                command->type = openxc_ControlCommand_Type_PLATFORM;
            } else if(!strncmp(commandNameObject->valuestring,
                        DIAGNOSTIC_COMMAND_NAME, strlen(DIAGNOSTIC_COMMAND_NAME))) {
                deserializeDiagnostic(root, command);
            } else if(!strncmp(commandNameObject->valuestring,
                        PASSTHROUGH_COMMAND_NAME, strlen(PASSTHROUGH_COMMAND_NAME))) {
                deserializePassthrough(root, command);
            } else if(!strncmp(commandNameObject->valuestring,
                        PREDEFINED_OBD2_REQUESTS_COMMAND_NAME,
                            strlen(PREDEFINED_OBD2_REQUESTS_COMMAND_NAME))) {
                deserializePredefinedObd2RequestsCommand(root, command);
            } else if(!strncmp(commandNameObject->valuestring,
                        ACCEPTANCE_FILTER_BYPASS_COMMAND_NAME,
                        strlen(ACCEPTANCE_FILTER_BYPASS_COMMAND_NAME))) {
                deserializeAfBypass(root, command);
            } else if(!strncmp(commandNameObject->valuestring,
                        PAYLOAD_FORMAT_COMMAND_NAME,
                        strlen(PAYLOAD_FORMAT_COMMAND_NAME))) {
                deserializePayloadFormat(root, command);
            } 
            else if(!strncmp(commandNameObject->valuestring,
                        MODEM_CONFIGURATION_COMMAND_NAME,
                        strlen(MODEM_CONFIGURATION_COMMAND_NAME))) {
                deserializeModemConfiguration(root, command);
            }
            else if(!strncmp(commandNameObject->valuestring,
                        RTC_CONFIGURATION_COMMAND_NAME,
                        strlen(RTC_CONFIGURATION_COMMAND_NAME))) {
                deserializeRTCConfiguration(root, command);
            }
            else if(!strncmp(commandNameObject->valuestring,
                        SD_MOUNT_STATUS_COMMAND_NAME,
                        strlen(SD_MOUNT_STATUS_COMMAND_NAME))) {
                command->type = openxc_ControlCommand_Type_SD_MOUNT_STATUS;
            }
            else {
                debug("Unrecognized command: %s", commandNameObject->valuestring);
            }
        } else {
            cJSON* nameObject = cJSON_GetObjectItem(root, "name");
            if(nameObject == NULL) {
                deserializeCan(root, message);
            } else {
                deserializeSimple(root, message);
            }
        }
        cJSON_Delete(root);
    }

    return messageLength;
}

int openxc::payload::json::serialize(openxc_VehicleMessage* message,
        uint8_t payload[], size_t length) {
    cJSON* root = cJSON_CreateObject();
    size_t finalLength = 0;
    if(root != NULL) {
        bool status = true;
        if(message->type != openxc_VehicleMessage_Type_UNUSED) {
            cJSON_AddNumberToObject(root, "timestamp", message->timestamp);
        }
        if(message->type == openxc_VehicleMessage_Type_SIMPLE) {
            status = serializeSimple(message, root);
        } else if(message->type == openxc_VehicleMessage_Type_CAN) {
            status = serializeCan(message, root);
        } else if(message->type == openxc_VehicleMessage_Type_DIAGNOSTIC) {
            status = serializeDiagnostic(message, root);
        } else if(message->type == openxc_VehicleMessage_Type_COMMAND_RESPONSE) {
            status = serializeCommandResponse(message, root);
        } else {
            debug("Unrecognized message type -- not sending");
        }

        char* serialized = cJSON_PrintUnformatted(root);
        if(status && serialized != NULL) {
            // set the length to the strlen + 1, so we include the NULL
            // character as a delimiter
            finalLength = MIN(length, strlen(serialized) + 1);
            memcpy(payload, serialized, finalLength);

            free(serialized);
        } else {
            debug("Converting JSON to string failed -- possibly OOM");
        }

        cJSON_Delete(root);
    } else {
        debug("JSON object is NULL -- probably OOM");
    }
    return finalLength;
}

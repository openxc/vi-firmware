#include <payload/payload.h>
#include <payload/json.h>
#include <util/log.h>
#include <cJSON.h>
#include <stdlib.h>
#include <sys/param.h>
#include <stdio.h>

namespace payload = openxc::payload;

using openxc::util::log::debug;

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

const char openxc::payload::json::VERSION_COMMAND_NAME[] = "version";
const char openxc::payload::json::DEVICE_ID_COMMAND_NAME[] = "device_id";
const char openxc::payload::json::DIAGNOSTIC_COMMAND_NAME[] = "diagnostic";

const char openxc::payload::json::COMMAND_RESPONSE_FIELD_NAME[] = "command_response";
const char openxc::payload::json::COMMAND_RESPONSE_MESSAGE_FIELD_NAME[] = "message";

const char openxc::payload::json::BUS_FIELD_NAME[] = "bus";
const char openxc::payload::json::ID_FIELD_NAME[] = "id";
const char openxc::payload::json::DATA_FIELD_NAME[] = "data";
const char openxc::payload::json::NAME_FIELD_NAME[] = "name";
const char openxc::payload::json::VALUE_FIELD_NAME[] = "value";
const char openxc::payload::json::EVENT_FIELD_NAME[] = "event";

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

    if(message->diagnostic_response.has_pid) {
        cJSON_AddNumberToObject(root, payload::json::DIAGNOSTIC_PID_FIELD_NAME,
                message->diagnostic_response.pid);
    }

    if(message->diagnostic_response.has_negative_response_code) {
        cJSON_AddNumberToObject(root, payload::json::DIAGNOSTIC_NRC_FIELD_NAME,
                message->diagnostic_response.negative_response_code);
    }

    if(message->diagnostic_response.has_value) {
        cJSON_AddNumberToObject(root, payload::json::DIAGNOSTIC_VALUE_FIELD_NAME,
                message->diagnostic_response.value);
    } else if(message->diagnostic_response.has_payload) {
        char encodedData[67];
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
    } else if(message->command_response.type == openxc_ControlCommand_Type_DIAGNOSTIC) {
        typeString = payload::json::DIAGNOSTIC_COMMAND_NAME;
    } else {
        return false;
    }

    cJSON_AddStringToObject(root, payload::json::COMMAND_RESPONSE_FIELD_NAME,
            typeString);
    if(message->command_response.has_message) {
        cJSON_AddStringToObject(root,
                payload::json::COMMAND_RESPONSE_MESSAGE_FIELD_NAME,
                message->command_response.message);
    }
    return true;
}

static bool serializeRaw(openxc_VehicleMessage* message, cJSON* root) {
    cJSON_AddNumberToObject(root, payload::json::BUS_FIELD_NAME,
            message->raw_message.bus);
    cJSON_AddNumberToObject(root, payload::json::ID_FIELD_NAME,
            message->raw_message.message_id);

    char encodedData[67];
    const char* maxAddress = encodedData + sizeof(encodedData);
    char* encodedDataIndex = encodedData;
    encodedDataIndex += sprintf(encodedDataIndex, "0x");
    for(uint8_t i = 0; i < message->raw_message.data.size &&
            encodedDataIndex < maxAddress; i++) {
        encodedDataIndex += snprintf(encodedDataIndex,
                maxAddress - encodedDataIndex,
                "%02x", message->raw_message.data.bytes[i]);
    }
    cJSON_AddStringToObject(root, payload::json::DATA_FIELD_NAME,
            encodedData);
    return true;
}

static cJSON* serializeDynamicField(openxc_DynamicField* field) {
    cJSON* value = NULL;
    if(field->has_numeric_value) {
        value = cJSON_CreateNumber(field->numeric_value);
    } else if(field->has_boolean_value) {
        value = cJSON_CreateBool(field->boolean_value);
    } else if(field->has_string_value) {
        value = cJSON_CreateString(field->string_value);
    }
    return value;
}

static bool serializeTranslated(openxc_VehicleMessage* message, cJSON* root) {
    const char* name = message->translated_message.name;
    cJSON_AddStringToObject(root, payload::json::NAME_FIELD_NAME, name);

    cJSON* value = NULL;
    if(message->translated_message.has_value) {
        value = serializeDynamicField(&message->translated_message.value);
        if(value != NULL) {
            cJSON_AddItemToObject(root, payload::json::VALUE_FIELD_NAME, value);
        }
    }

    cJSON* event = NULL;
    if(message->translated_message.has_event) {
        event = serializeDynamicField(&message->translated_message.event);
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

static void deserializeDiagnostic(cJSON* root, openxc_ControlCommand* command) {
    command->has_type = true;
    command->type = openxc_ControlCommand_Type_DIAGNOSTIC;
    command->has_diagnostic_request = true;

    cJSON* request = cJSON_GetObjectItem(root, "request");
    if(request != NULL) {
        cJSON* element = cJSON_GetObjectItem(request, "bus");
        if(element != NULL) {
            command->diagnostic_request.has_bus = true;
            command->diagnostic_request.bus = element->valueint;
        }

        element = cJSON_GetObjectItem(request, "mode");
        if(element != NULL) {
            command->diagnostic_request.has_mode = true;
            command->diagnostic_request.mode = element->valueint;
        }

        element = cJSON_GetObjectItem(request, "id");
        if(element != NULL) {
            command->diagnostic_request.has_message_id = true;
            command->diagnostic_request.message_id = element->valueint;
        }

        element = cJSON_GetObjectItem(request, "pid");
        if(element != NULL) {
            command->diagnostic_request.has_pid = true;
            command->diagnostic_request.pid = element->valueint;
        }

        element = cJSON_GetObjectItem(request, "payload");
        if(element != NULL) {
            command->diagnostic_request.has_payload = true;
            command->diagnostic_request.payload.size = dehexlify(
                    element->valuestring,
                    command->diagnostic_request.payload.bytes,
                    sizeof(((openxc_DiagnosticRequest*)0)->payload.bytes));
        }

        element = cJSON_GetObjectItem(request, "parse_payload");
        if(element != NULL) {
            command->diagnostic_request.has_parse_payload = true;
            command->diagnostic_request.parse_payload = bool(element->valueint);
        }

        element = cJSON_GetObjectItem(request, "multiple_responses");
        if(element != NULL) {
            command->diagnostic_request.has_multiple_responses = true;
            command->diagnostic_request.multiple_responses = bool(element->valueint);
        }

        element = cJSON_GetObjectItem(request, "factor");
        if(element != NULL) {
            command->diagnostic_request.has_factor = true;
            command->diagnostic_request.factor = element->valuedouble;
        }

        element = cJSON_GetObjectItem(request, "offset");
        if(element != NULL) {
            command->diagnostic_request.has_offset = true;
            command->diagnostic_request.offset = element->valuedouble;
        }

        element = cJSON_GetObjectItem(request, "frequency");
        if(element != NULL) {
            command->diagnostic_request.has_frequency = true;
            command->diagnostic_request.frequency = element->valuedouble;
        }
    }
}

static bool deserializeDynamicField(cJSON* element, openxc_DynamicField* field) {
    bool status = true;
    field->has_type = true;
    switch(element->type) {
        case cJSON_String:
            field->type = openxc_DynamicField_Type_STRING;
            field->has_string_value = true;
            strcpy(field->string_value, element->valuestring);
            break;
        case cJSON_False:
        case cJSON_True:
            field->type = openxc_DynamicField_Type_BOOL;
            field->has_boolean_value = true;
            field->boolean_value = bool(element->valueint);
            break;
        case cJSON_Number:
            field->type = openxc_DynamicField_Type_NUM;
            field->has_numeric_value = true;
            field->numeric_value = element->valuedouble;
            break;
        default:
            debug("Unsupported type in value field: %d", element->type);
            field->has_type = false;
            status = false;
            break;
    }
    return status;
}

static void deserializeTranslated(cJSON* root, openxc_VehicleMessage* message) {
    message->has_type = true;
    message->type = openxc_VehicleMessage_Type_TRANSLATED;
    message->has_translated_message = true;
    openxc_TranslatedMessage* translatedMessage = &message->translated_message;

    cJSON* element = cJSON_GetObjectItem(root, "name");
    if(element != NULL && element->type == cJSON_String) {
        translatedMessage->has_name = true;
        strcpy(translatedMessage->name, element->valuestring);
    }

    element = cJSON_GetObjectItem(root, "value");
    if(element != NULL) {
        if(deserializeDynamicField(element, &translatedMessage->value)) {
            translatedMessage->has_value = true;
        }
    }

    element = cJSON_GetObjectItem(root, "event");
    if(element != NULL) {
        if(deserializeDynamicField(element, &translatedMessage->event)) {
            translatedMessage->has_event = true;
        }
    }

    translatedMessage->has_type = true;
    if(translatedMessage->has_event) {
        if(translatedMessage->event.has_string_value) {
            translatedMessage->type = openxc_TranslatedMessage_Type_EVENTED_STRING;
        } else if(translatedMessage->event.has_numeric_value) {
            translatedMessage->type = openxc_TranslatedMessage_Type_EVENTED_NUM;
        } else if(translatedMessage->event.has_boolean_value) {
            translatedMessage->type = openxc_TranslatedMessage_Type_EVENTED_BOOL;
        }
    } else {
        if(translatedMessage->value.has_string_value) {
            translatedMessage->type = openxc_TranslatedMessage_Type_STRING;
        } else if(translatedMessage->value.has_numeric_value) {
            translatedMessage->type = openxc_TranslatedMessage_Type_NUM;
        } else if(translatedMessage->value.has_boolean_value) {
            translatedMessage->type = openxc_TranslatedMessage_Type_BOOL;
        }
    }
}

static void deserializeRaw(cJSON* root, openxc_VehicleMessage* message) {
    message->has_type = true;
    message->type = openxc_VehicleMessage_Type_RAW;
    message->has_raw_message = true;
    openxc_RawMessage* rawMessage = &message->raw_message;

    cJSON* element = cJSON_GetObjectItem(root, "id");
    if(element != NULL) {
        rawMessage->has_message_id = true;
        rawMessage->message_id = element->valueint;

        element = cJSON_GetObjectItem(root, "data");
        if(element != NULL) {
            rawMessage->has_data = true;
            rawMessage->data.size = dehexlify(
                    element->valuestring,
                    rawMessage->data.bytes,
                    sizeof(((openxc_RawMessage*)0)->data.bytes));
        }

        element = cJSON_GetObjectItem(root, "bus");
        if(element != NULL) {
            rawMessage->has_bus = true;
            rawMessage->bus = element->valueint;
        }
    } else {
        message->has_raw_message = false;
    }
}

bool openxc::payload::json::deserialize(uint8_t payload[], size_t length,
        openxc_VehicleMessage* message) {
    cJSON *root = cJSON_Parse((char*)payload);
    if(root == NULL) {
        return false;
    }

    message->has_type = true;
    cJSON* commandNameObject = cJSON_GetObjectItem(root, "command");
    if(commandNameObject != NULL) {
        message->has_type = true;
        message->type = openxc_VehicleMessage_Type_CONTROL_COMMAND;
        message->has_control_command = true;
        openxc_ControlCommand* command = &message->control_command;

        if(!strncmp(commandNameObject->valuestring, VERSION_COMMAND_NAME,
                    strlen(VERSION_COMMAND_NAME))) {
            command->has_type = true;
            command->type = openxc_ControlCommand_Type_VERSION;
        } else if(!strncmp(commandNameObject->valuestring,
                    DEVICE_ID_COMMAND_NAME, strlen(DEVICE_ID_COMMAND_NAME))) {
            command->has_type = true;
            command->type = openxc_ControlCommand_Type_DEVICE_ID;
        } else if(!strncmp(commandNameObject->valuestring,
                    DIAGNOSTIC_COMMAND_NAME, strlen(DIAGNOSTIC_COMMAND_NAME))) {
            deserializeDiagnostic(root, command);
        } else {
            debug("Unrecognized command: %s", commandNameObject->valuestring);
            message->has_control_command = false;
        }
    } else {
        cJSON* nameObject = cJSON_GetObjectItem(root, "name");
        if(nameObject == NULL) {
            deserializeRaw(root, message);
        } else {
            deserializeTranslated(root, message);
        }
    }
    cJSON_Delete(root);
    return true;
}

int openxc::payload::json::serialize(openxc_VehicleMessage* message,
        uint8_t payload[], size_t length) {
    cJSON* root = cJSON_CreateObject();
    size_t finalLength = 0;
    if(root != NULL) {
        bool status = true;
        if(message->type == openxc_VehicleMessage_Type_TRANSLATED) {
            status = serializeTranslated(message, root);
        } else if(message->type == openxc_VehicleMessage_Type_RAW) {
            status = serializeRaw(message, root);
        } else if(message->type == openxc_VehicleMessage_Type_DIAGNOSTIC) {
            status = serializeDiagnostic(message, root);
        } else if(message->type == openxc_VehicleMessage_Type_COMMAND_RESPONSE) {
            status = serializeCommandResponse(message, root);
        } else {
            debug("Unrecognized message type -- not sending");
        }

        char* message = cJSON_PrintUnformatted(root);
        if(status && message != NULL) {
            char messageWithDelimeter[strlen(message) + 3];
            strncpy(messageWithDelimeter, message, strlen(message));
            messageWithDelimeter[strlen(message)] = '\0';
            strncat(messageWithDelimeter, "\r\n", 2);

            finalLength = MIN(length, strlen(messageWithDelimeter));
            memcpy(payload, messageWithDelimeter, finalLength);

            free(message);
        } else {
            debug("Converting JSON to string failed -- possibly OOM");
        }

        cJSON_Delete(root);
    } else {
        debug("JSON object is NULL -- probably OOM");
    }
    return finalLength;
}

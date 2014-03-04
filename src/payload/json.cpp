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

static bool serializeDiagnostic(openxc_VehicleMessage* message, cJSON* root) {
    cJSON_AddNumberToObject(root, can::read::BUS_FIELD_NAME,
            message->diagnostic_response.bus);
    cJSON_AddNumberToObject(root, can::read::ID_FIELD_NAME,
            message->diagnostic_response.message_id);
    cJSON_AddNumberToObject(root, can::read::DIAGNOSTIC_MODE_FIELD_NAME,
            message->diagnostic_response.mode);
    cJSON_AddBoolToObject(root, can::read::DIAGNOSTIC_SUCCESS_FIELD_NAME,
            message->diagnostic_response.success);

    if(message->diagnostic_response.has_pid) {
        cJSON_AddNumberToObject(root, can::read::DIAGNOSTIC_PID_FIELD_NAME,
                message->diagnostic_response.pid);
    }

    if(message->diagnostic_response.has_negative_response_code) {
        cJSON_AddNumberToObject(root, can::read::DIAGNOSTIC_NRC_FIELD_NAME,
                message->diagnostic_response.negative_response_code);
    }

    if(message->diagnostic_response.has_value) {
        cJSON_AddNumberToObject(root, can::read::DIAGNOSTIC_VALUE_FIELD_NAME,
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
        cJSON_AddStringToObject(root, can::read::DIAGNOSTIC_PAYLOAD_FIELD_NAME,
                encodedData);
    }
    return true;
}

static bool serializeRaw(openxc_VehicleMessage* message, cJSON* root) {
    cJSON_AddNumberToObject(root, can::read::BUS_FIELD_NAME,
            message->raw_message.bus);
    cJSON_AddNumberToObject(root, can::read::ID_FIELD_NAME,
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
    cJSON_AddStringToObject(root, can::read::DATA_FIELD_NAME,
            encodedData);
    return true;
}


static bool serializeTranslated(openxc_VehicleMessage* message, cJSON* root) {
    const char* name = message->translated_message.name;
    cJSON* value = NULL;
    if(message->translated_message.has_numeric_value) {
        value = cJSON_CreateNumber(
                message->translated_message.numeric_value);
    } else if(message->translated_message.has_boolean_value) {
        value = cJSON_CreateBool(
                message->translated_message.boolean_value);
    } else if(message->translated_message.has_string_value) {
        value = cJSON_CreateString(
                message->translated_message.string_value);
    }

    cJSON* event = NULL;
    if(message->translated_message.has_numeric_event) {
        event = cJSON_CreateNumber(
                message->translated_message.numeric_event);
    } else if(message->translated_message.has_boolean_event) {
        event = cJSON_CreateBool(
                message->translated_message.boolean_event);
    } else if(message->translated_message.has_string_event) {
        event = cJSON_CreateString(
                message->translated_message.string_event);
    }

    cJSON_AddStringToObject(root, can::read::NAME_FIELD_NAME, name);
    cJSON_AddItemToObject(root, can::read::VALUE_FIELD_NAME, value);
    if(event != NULL) {
        cJSON_AddItemToObject(root, can::read::EVENT_FIELD_NAME,
                event);
    }
    return true;
}

static bool deserializeDiagnostic(cJSON* root, openxc_ControlCommand* command) {
    command->type = openxc_ControlCommand_Type_DIAGNOSTIC;
    command->has_diagnostic_request = true;

    cJSON* request = cJSON_GetObjectItem(root, "request");
    if(request == NULL) {
        return false;
    }

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
        // TODO need to go from hex string to byte array.
        command->diagnostic_request.payload.size = 0;
        // mempcy(command->diagnostic_request.payload.bytes,
                // xxx, xxx.length);
    }

    element = cJSON_GetObjectItem(request, "parse_payload");
    if(element != NULL) {
        command->diagnostic_request.has_parse_payload = true;
        command->diagnostic_request.parse_payload = bool(element->valueint);
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

    return true;
}

bool openxc::payload::json::deserialize(uint8_t payload[], size_t length,
        openxc_VehicleMessage* message) {
    cJSON *root = cJSON_Parse((char*)payload);
    if(root == NULL) {
        return false;
    }

    // TODO need to handle deserializing regular vehicle data messages, for
    // writes
    bool status = true;
    cJSON* commandNameObject = cJSON_GetObjectItem(root, "command");
    if(commandNameObject != NULL) {
        message->has_type = true;
        openxc_ControlCommand* command = &message->control_command;

        command->has_type = true;
        if(!strncmp(commandNameObject->valuestring, VERSION_COMMAND_NAME,
                    strlen(VERSION_COMMAND_NAME))) {
            command->type = openxc_ControlCommand_Type_VERSION;
        } else if(!strncmp(commandNameObject->valuestring,
                    DEVICE_ID_COMMAND_NAME, strlen(DEVICE_ID_COMMAND_NAME))) {
            command->type = openxc_ControlCommand_Type_DEVICE_ID;
        } else if(!strncmp(commandNameObject->valuestring,
                    DIAGNOSTIC_COMMAND_NAME, strlen(DIAGNOSTIC_COMMAND_NAME))) {
            status = deserializeDiagnostic(root, command);
        } else {
            debug("Unrecognized command: %s", commandNameObject->valuestring);
            status = false;
        }
    }
    cJSON_Delete(root);
    return status;
}

int openxc::payload::json::serialize(openxc_VehicleMessage* message,
        uint8_t payload[], size_t length) {
    cJSON* root = cJSON_CreateObject();
    size_t finalLength = 0;
    if(root != NULL) {
        if(message->type == openxc_VehicleMessage_Type_TRANSLATED) {
            serializeTranslated(message, root);
        } else if(message->type == openxc_VehicleMessage_Type_RAW) {
            serializeRaw(message, root);
        } else if(message->type == openxc_VehicleMessage_Type_DIAGNOSTIC) {
            serializeDiagnostic(message, root);
        } else {
            debug("Unrecognized message type -- not sending");
        }

        char* message = cJSON_PrintUnformatted(root);
        if(message != NULL) {
            char messageWithDelimeter[strlen(message) + 3];
            strncpy(messageWithDelimeter, message, strlen(message));
            messageWithDelimeter[strlen(message)] = '\0';
            strncat(messageWithDelimeter, "\r\n", 2);

            finalLength = MIN(length, strlen(messageWithDelimeter));
            memcpy(payload, messageWithDelimeter, finalLength);

            free(message);
        } else {
            debug("Converting JSON to string failed -- probably OOM");
        }

        cJSON_Delete(root);
    } else {
        debug("JSON object is NULL -- probably OOM");
    }
    return finalLength;
}

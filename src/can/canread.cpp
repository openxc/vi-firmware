#include "can/canread.h"
#include <stdlib.h>
#include "util/log.h"
#include "util/timer.h"
#include "pb_encode.h"
#include "../lights.h"
extern "C"{
#include "fat_filelib.h"
}
#include "mediaIO.h"

using openxc::util::bitfield::getBitField;
using openxc::util::log::debugNoNewline;

namespace time = openxc::util::time;
namespace pipeline = openxc::pipeline;
namespace lights = openxc::lights;

const char openxc::can::read::BUS_FIELD_NAME[] = "bus";
const char openxc::can::read::ID_FIELD_NAME[] = "id";
const char openxc::can::read::DATA_FIELD_NAME[] = "data";
const char openxc::can::read::NAME_FIELD_NAME[] = "name";
const char openxc::can::read::VALUE_FIELD_NAME[] = "value";
const char openxc::can::read::EVENT_FIELD_NAME[] = "event";




/* Private: Serialize the root JSON object to a string (ending with a newline)
 * and send it to the pipeline.
 *
 * root - The JSON object to send.
 * pipeline - The pipeline to send on.
 */

void sendJSON(cJSON* root, Pipeline* pipeline) {
    if(root == NULL) {
        debug("JSON object is NULL -- probably OOM");
    } else {
        char* message = cJSON_PrintUnformatted(root);
		char messageWithDelimeter[strlen(message) + 3];
        strncpy(messageWithDelimeter, message, strlen(message));
        messageWithDelimeter[strlen(message)] = '\0';
        strncat(messageWithDelimeter, "\r\n", 2);
		if(message != NULL) {
			#if defined(DATA_LOGGER)
				writeToUSB(message);
			#else
				pipeline::sendMessage(pipeline, (uint8_t*) messageWithDelimeter,
						strlen(messageWithDelimeter));
			#endif
		} else {
			debug("Converting JSON to string failed -- probably OOM");
		}
        free(message);
    }
}

/* Private: Serialize the object to a string/protobuf
 * and send it to the pipeline.
 *
 * message - The message to send, in a struct.
 * pipeline - The pipeline to send on.
 */
void sendProtobuf(openxc_VehicleMessage* message, Pipeline* pipeline) {
    if(message == NULL) {
        debug("Message object is NULL");
        return;
    }
    uint8_t buffer[openxc_VehicleMessage_size + 1];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool status = true;
    status = pb_encode_delimited(&stream, openxc_VehicleMessage_fields, message);
    if(status) {
        pipeline::sendMessage(pipeline, buffer, stream.bytes_written);
    } else {
        debug("Error encoding protobuf: %s", PB_GET_ERROR(&stream));
    }
}

/* Private: Combine the given name and value into a JSON object (conforming to
 * the OpenXC standard) and send it out to the pipeline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The numeric_value, string or booelan for the value field of the OpenXC
 *     message.
 * event - (Optional) The event for the event field of the OpenXC message.
 * pipeline - The pipeline to send on.
 */
void sendJsonMessage(const char* name, cJSON* value, cJSON* event,
        Pipeline* pipeline) {
    using openxc::can::read::NAME_FIELD_NAME;
    using openxc::can::read::VALUE_FIELD_NAME;
    using openxc::can::read::EVENT_FIELD_NAME;
    cJSON *root = cJSON_CreateObject();
    if(root != NULL) {
		//char messageWithDelimeter[20];
		//char str2[20];
		//strcpy (messageWithDelimeter,"Name: ");
		//strcpy (str2,name);
		//strncat (messageWithDelimeter, str2, 6);
		//debug(messageWithDelimeter);
	     //       pipeline::sendMessage(pipeline, (uint8_t*) messageWithDelimeter,
		//				strlen(messageWithDelimeter));
		#ifdef DATA_LOGGER
		cJSON* counter = cJSON_CreateNumber(openxc::util::time::systemTimeMs());
		cJSON_AddItemToObject(root, "counter", counter);
		#endif
        cJSON_AddStringToObject(root, NAME_FIELD_NAME, name);
        cJSON_AddItemToObject(root, VALUE_FIELD_NAME, value);
		
        if(event != NULL) {
            cJSON_AddItemToObject(root, EVENT_FIELD_NAME, event);
        }
        sendJSON(root, pipeline);
		cJSON_Delete(root);
    } else {
        debug("Unable to allocate a cJSON object - probably OOM");
    }
}
/*
 *
 * TODO push responsibility for encoding to output formats to the pipeline - we
 * just send along a generic object, the oepnxc_VehicleMessage struct might be a
 * good candidate.
 */
void sendMessage(openxc_VehicleMessage* message, Pipeline* pipeline) {
    if(pipeline->outputFormat == pipeline::PROTO) {
        sendProtobuf(message, pipeline);
    } else {
        // TODO is this the right place to do this?
        const char* name;
		const char* ignition = "ignition_status";
		const char* run = "run";
		const char* off = "off";
        cJSON* value;
        cJSON* event = NULL;
        switch(message->type) {
            case openxc_VehicleMessage_Type_TRANSLATED:
                name = message->translated_message.name;
                if(message->translated_message.has_numeric_value) {
                    value = cJSON_CreateNumber(message->translated_message.numeric_value);
                } else if(message->translated_message.has_boolean_value) {
                    value = cJSON_CreateBool(message->translated_message.boolean_value);
                } else if(message->translated_message.has_string_value) {
                    value = cJSON_CreateString(message->translated_message.string_value);
                }

                if(message->translated_message.has_numeric_event) {
                    event = cJSON_CreateNumber(message->translated_message.numeric_event);
                } else if(message->translated_message.has_boolean_event) {
                    event = cJSON_CreateBool(message->translated_message.boolean_event);
                } else if(message->translated_message.has_string_event) {
                    event = cJSON_CreateString(message->translated_message.string_event);
                }
                break;
            default:
                debug("Unrecognized message type, can't output JSON");
                // TODO handle raw message type here?
                break;
        }
		if(getWriteStatus()){
			sendJsonMessage(name, value, event, pipeline);
		}
	}
}

float openxc::can::read::preTranslate(CanSignal* signal, uint64_t data,
        bool* send) {
    float value = decodeSignal(signal, data);

    if(time::shouldTick(&signal->frequencyClock) ||
            (value != signal->lastValue && signal->forceSendChanged)) {
        if(send && (!signal->received || signal->sendSame ||
                    value != signal->lastValue)) {
            signal->received = true;
        } else {
            *send = false;
        }
    } else {
        *send = false;
    }
    return value;
}

void openxc::can::read::postTranslate(CanSignal* signal, float value) {
    signal->lastValue = value;
}

float openxc::can::read::decodeSignal(CanSignal* signal, uint64_t data) {
    uint64_t rawValue = getBitField(data, signal->bitPosition,
            signal->bitSize, true);
    return rawValue * signal->factor + signal->offset;
}

float openxc::can::read::passthroughHandler(CanSignal* signal,
        CanSignal* signals, int signalCount, float value, bool* send) {
    return value;
}

bool openxc::can::read::booleanHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    return value == 0.0 ? false : true;
}

float openxc::can::read::ignoreHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    *send = false;
    return value;
}

const char* openxc::can::read::stateHandler(CanSignal* signal,
        CanSignal* signals, int signalCount, float value, bool* send) {
    const CanSignalState* signalState = lookupSignalState(value, signal,
            signals, signalCount);
    if(signalState != NULL) {
        return signalState->name;
    }
    *send = false;
    return NULL;
}

void openxc::can::read::sendNumericalMessage(const char* name, float value,
        Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_TRANSLATED;
    message.has_translated_message = true;
    message.translated_message = {0};
    message.translated_message.has_name = true;
    strcpy(message.translated_message.name, name);
    message.translated_message.has_type = true;
    message.translated_message.type = openxc_TranslatedMessage_Type_NUM;
    message.translated_message.has_numeric_value = true;
    message.translated_message.numeric_value = value;

    sendMessage(&message, pipeline);
}

void openxc::can::read::sendBooleanMessage(const char* name, bool value,
        Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_TRANSLATED;
    message.has_translated_message = true;
    message.translated_message = {0};
    message.translated_message.has_name = true;
    strcpy(message.translated_message.name, name);
    message.translated_message.has_type = true;
    message.translated_message.type = openxc_TranslatedMessage_Type_BOOL;
    message.translated_message.has_boolean_value = true;
    message.translated_message.boolean_value = value;

    sendMessage(&message, pipeline);
}

void openxc::can::read::sendStringMessage(const char* name, const char* value,
        Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_TRANSLATED;
    message.has_translated_message = true;
    message.translated_message = {0};
    message.translated_message.has_name = true;
    strcpy(message.translated_message.name, name);
    message.translated_message.has_type = true;
    message.translated_message.type = openxc_TranslatedMessage_Type_STRING;
    message.translated_message.has_string_value = true;
    strcpy(message.translated_message.string_value, value);

    sendMessage(&message, pipeline);
}

void openxc::can::read::sendEventedFloatMessage(const char* name,
        const char* value, float event,
        Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_TRANSLATED;
    message.has_translated_message = true;
    message.translated_message = {0};
    message.translated_message.has_name = true;
    strcpy(message.translated_message.name, name);
    message.translated_message.has_type = true;
    message.translated_message.type = openxc_TranslatedMessage_Type_EVENTED_NUM;
    message.translated_message.has_string_value = true;
    strcpy(message.translated_message.string_value, value);
    message.translated_message.has_numeric_event = true;
    message.translated_message.numeric_event = event;

    sendMessage(&message, pipeline);
}

void openxc::can::read::sendEventedBooleanMessage(const char* name,
        const char* value, bool event, Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_TRANSLATED;
    message.has_translated_message = true;
    message.translated_message = {0};
    message.translated_message.has_name = true;
    strcpy(message.translated_message.name, name);
    message.translated_message.has_type = true;
    message.translated_message.type = openxc_TranslatedMessage_Type_EVENTED_BOOL;
    message.translated_message.has_string_value = true;
    strcpy(message.translated_message.string_value, value);
    message.translated_message.has_boolean_event = true;
    message.translated_message.boolean_event = event;

    sendMessage(&message, pipeline);
}

void openxc::can::read::sendEventedStringMessage(const char* name,
        const char* value, const char* event, Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_TRANSLATED;
    message.has_translated_message = true;
    message.translated_message = {0};
    message.translated_message.has_name = true;
    strcpy(message.translated_message.name, name);
    message.translated_message.has_type = true;
    message.translated_message.type = openxc_TranslatedMessage_Type_EVENTED_STRING;
    message.translated_message.has_string_value = true;
    strcpy(message.translated_message.string_value, value);
    message.translated_message.has_string_event = true;
    strcpy(message.translated_message.string_event, event);

    sendMessage(&message, pipeline);
}

void passthroughMessageJson(CanBus* bus, uint32_t id,
        uint64_t data, CanMessageDefinition* messages, int messageCount,
        Pipeline* pipeline) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, openxc::can::read::BUS_FIELD_NAME, bus->address);
    char encodedData[67];
    union {
        uint64_t whole;
        uint8_t bytes[8];
    } combined;
    combined.whole = data;


    sprintf(encodedData, "0x%02x%02x%02x%02x%02x%02x%02x%02x",
            combined.bytes[0],
            combined.bytes[1],
            combined.bytes[2],
            combined.bytes[3],
            combined.bytes[4],
            combined.bytes[5],
            combined.bytes[6],
            combined.bytes[7]);
    cJSON_AddStringToObject(root, openxc::can::read::DATA_FIELD_NAME, encodedData);
	cJSON_AddNumberToObject(root, openxc::can::read::ID_FIELD_NAME, id);
	//Add counter
	#ifdef DATA_LOGGER
	cJSON* counter = cJSON_CreateNumber(openxc::util::time::systemTimeMs());
	cJSON_AddItemToObject(root, "counter", counter);
	#endif
	sendJSON(root, pipeline);
	cJSON_Delete(root);
}

void passthroughMessageProtobuf(CanBus* bus, uint32_t id,
        uint64_t data, CanMessageDefinition* messages, int messageCount,
        Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_RAW;
    message.has_raw_message = true;
    message.raw_message = {0};
    message.raw_message.has_message_id = true;
    message.raw_message.message_id = id;
    message.raw_message.has_bus = true;
    message.raw_message.bus = bus->address;
    message.raw_message.has_data = true;
    message.raw_message.data = data;

    sendProtobuf(&message, pipeline);
}

void openxc::can::read::passthroughMessage(CanBus* bus, uint32_t id,
        uint64_t data, CanMessageDefinition* messages, int messageCount,
        Pipeline* pipeline) {
    bool send = true;
	
    CanMessageDefinition* message = lookupMessageDefinition(bus, id, messages,
            messageCount);
			
    if(message == NULL) {
        debug("Adding new message definition for message %d on bus %d",
                id, bus->address);
		#ifndef DATA_LOGGER
        send = registerMessageDefinition(bus, id, messages, messageCount);
		#endif
    } else if(time::shouldTick(&message->frequencyClock) ||
            (data != message->lastValue && message->forceSendChanged)) {
        send = true;
    } else {
        send = false;
    }

    if(send) {
        if(pipeline->outputFormat == pipeline::PROTO) {
            passthroughMessageProtobuf(bus, id, data, messages, messageCount,
                    pipeline);
        } else {
            passthroughMessageJson(bus, id, data, messages, messageCount,
                    pipeline);
        }
    }

    if(message != NULL) {
        message->lastValue = data;
    }
}

void openxc::can::read::translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    bool send = true;
    float value = preTranslate(signal, data, &send);
    float processedValue = handler(signal, signals, signalCount, value, &send);
    if(send) {
        sendNumericalMessage(signal->genericName, processedValue, pipeline);
    }
    postTranslate(signal, value);
}

void openxc::can::read::translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data,
        const char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    bool send = true;
    float value = preTranslate(signal, data, &send);
    const char* stringValue = handler(signal, signals, signalCount, value,
            &send);
    if(stringValue != NULL && send) {
        sendStringMessage(signal->genericName, stringValue, pipeline);
    }
    postTranslate(signal, value);
}

void openxc::can::read::translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    bool send = true;
    float value = preTranslate(signal, data, &send);
    bool booleanValue = handler(signal, signals, signalCount, value, &send);
    if(send) {
        sendBooleanMessage(signal->genericName, booleanValue, pipeline);
    }
    postTranslate(signal, value);
}

void openxc::can::read::translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data, CanSignal* signals, int signalCount) {
    translateSignal(pipeline, signal, data, passthroughHandler, signals,
            signalCount);
}

#include "canutil_chipkit.h"
#include "usbutil.h"
#include "cJSON.h"

void configureFilters(CAN *can_module, CanFilterMask* filterMasks,
        int filterMaskCount, CanFilter* filters, int filterCount) {
    Serial.print("Configuring ");
    Serial.print(filterMaskCount, DEC);
    Serial.print(" filter masks...  ");
    for(int i = 0; i < filterMaskCount; i++) {
        Serial.print("Configuring filter mask ");
        Serial.println(filterMasks[i].value, HEX);
        can_module->configureFilterMask(
                (CAN::FILTER_MASK) filterMasks[i].number,
                filterMasks[i].value, CAN::SID, CAN::FILTER_MASK_IDE_TYPE);
    }
    Serial.println("Done.");

    Serial.print("Configuring ");
    Serial.print(filterCount, DEC);
    Serial.print(" filters...  ");
    for(int i = 0; i < filterCount; i++) {
        can_module->configureFilter((CAN::FILTER) filters[i].number,
                filters[i].value, CAN::SID);
        can_module->linkFilterToChannel((CAN::FILTER) filters[i].number,
                (CAN::FILTER_MASK) filters[i].maskNumber,
                (CAN::CHANNEL) filters[i].channel);
        can_module->enableFilter((CAN::FILTER) filters[i].number, true);
    }
    Serial.println("Done.");
}

void initializeCan(CAN* bus, int address, int speed, uint8_t* messageArea) {
    Serial.print("Initializing CAN bus at ");
    Serial.println(address, DEC);
    CAN::BIT_CONFIG canBitConfig;

    /* Switch the CAN module ON and switch it to Configuration mode. Wait till
     * the switch is complete */
    bus->enableModule(true);
    bus->setOperatingMode(CAN::CONFIGURATION);
    while(bus->getOperatingMode() != CAN::CONFIGURATION);

    /* Configure the CAN Module Clock. The CAN::BIT_CONFIG data structure
     * is used for this purpose. The propagation, phase segment 1 and phase
     * segment 2 are configured to have 3TQ. The CANSetSpeed() function sets the
     * baud. */
    canBitConfig.phaseSeg2Tq            = CAN::BIT_3TQ;
    canBitConfig.phaseSeg1Tq            = CAN::BIT_3TQ;
    canBitConfig.propagationSegTq       = CAN::BIT_3TQ;
    canBitConfig.phaseSeg2TimeSelect    = CAN::TRUE;
    canBitConfig.sample3Time            = CAN::TRUE;
    canBitConfig.syncJumpWidth          = CAN::BIT_2TQ;
    bus->setSpeed(&canBitConfig, SYS_FREQ, speed);

    /* Assign the buffer area to the CAN module. */
    /* Note the size of each Channel area. It is 2 (Channels) * 8 (Messages
     * Buffers) 16 (bytes/per message buffer) bytes. Each CAN module should have
     * its own message area. */
    bus->assignMemoryBuffer(messageArea, 2 * 8 * 16);

    /* Configure channel 1 for RX and size of 8 message buffers and receive the
     * full message.
     */
    bus->configureChannelForRx(CAN::CHANNEL1, 8, CAN::RX_FULL_RECEIVE);

    int filterMaskCount;
    CanFilterMask* filterMasks = initializeFilterMasks(address,
            &filterMaskCount);
    int filterCount;
    CanFilter* filters = initializeFilters(address, &filterCount);
    configureFilters(bus, filterMasks, filterMaskCount, filters, filterCount);

    /* Enable interrupt and events. Enable the receive channel not empty
     * event (channel event) and the receive channel event (module event). The
     * interrrupt peripheral library is used to enable the CAN interrupt to the
     * CPU. */
    bus->enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);
    bus->enableModuleEvent(CAN::RX_EVENT, true);

    bus->setOperatingMode(CAN::LISTEN_ONLY);
    while(bus->getOperatingMode() != CAN::LISTEN_ONLY);

    Serial.println("Done.");
}

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    float processedValue = handler(signal, signals, signalCount, value, &send);

    if(signal->sendClock == signal->sendFrequency) {
        if(send && (signal->sendSame || !signal->received ||
                    value != signal->lastValue)) {
            signal->received = true;
            sendNumericalMessage(signal->genericName, processedValue,
                    usbDevice);
        }
        signal->sendClock = 0;
    } else {
        ++signal->sendClock;
    }
    signal->lastValue = value;
}

void sendJSON(cJSON* root, CanUsbDevice* usbDevice) {
    char *message = cJSON_PrintUnformatted(root);
    sendMessage(usbDevice, (uint8_t*) message, strlen(message));
    cJSON_Delete(root);
    free(message);
}

// TODO if we make value a void*, could this be used for all of the
// translateCanSignal functions? We can pass in the formats no problem, but the
// differently typed values mean you would need to repeat this entire function
// multiple files anyway. I'm leaving this function for now because it's useful
// in a custom handler.
void sendNumericalMessage(char* name, float value, CanUsbDevice* usbDevice) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, NAME_FIELD_NAME, name);
    cJSON_AddNumberToObject(root, VALUE_FIELD_NAME, value);

    sendJSON(root, usbDevice);
}

void sendBooleanMessage(char* name, bool value, CanUsbDevice* usbDevice) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, NAME_FIELD_NAME, name);
    Serial.println("creating bool");
    cJSON_AddItemToObject(root, VALUE_FIELD_NAME, cJSON_CreateBool(value));
    Serial.println("sending");
    sendJSON(root, usbDevice);
}

void sendStringMessage(char* name, char* value, CanUsbDevice* usbDevice) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, NAME_FIELD_NAME, name);
    cJSON_AddStringToObject(root, VALUE_FIELD_NAME, value);
    sendJSON(root, usbDevice);
}

void sendEventedBooleanMessage(char* name, char* value, bool event,
        CanUsbDevice* usbDevice) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, NAME_FIELD_NAME, name);
    cJSON_AddStringToObject(root, VALUE_FIELD_NAME, value);
    cJSON_AddItemToObject(root, EVENT_FIELD_NAME, cJSON_CreateBool(event));
    sendJSON(root, usbDevice);
}

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    char* stringValue = handler(signal, signals, signalCount, value, &send);

    if(signal->sendClock == signal->sendFrequency) {
        if(send && (signal->sendSame || !signal->received ||
                    value != signal->lastValue)) {
            signal->received = true;
            sendStringMessage(signal->genericName, stringValue, usbDevice);
        }
        signal->sendClock = 0;
    } else {
        ++signal->sendClock;
    }
    signal->lastValue = value;
}

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    bool booleanValue = handler(signal, signals, signalCount, value, &send);

    if(signal->sendClock == signal->sendFrequency) {
        if(send && (signal->sendSame || !signal->received ||
                    value != signal->lastValue)) {
            signal->received = true;
            sendBooleanMessage(signal->genericName, booleanValue, usbDevice);
        }
        signal->sendClock = 0;
    } else {
        ++signal->sendClock;
    }
    signal->lastValue = value;
}

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data, CanSignal* signals, int signalCount) {
    translateCanSignal(usbDevice, signal, data, passthroughHandler,
            signals, signalCount);
}

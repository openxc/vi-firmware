#include "canutil.h"
#include "usbutil.h"

extern CanFilter* initializeFilters(uint32_t, int*);

void configureFilters(CAN *can_module, CanFilter* filters, int filterCount) {
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

    int filterCount;
    CanFilter* filters = initializeFilters(address, &filterCount);
    configureFilters(bus, filters, filterCount);

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

float decodeCanSignal(CanSignal* signal, uint8_t* data) {
    unsigned long rawValue = getBitField(data, signal->bitPosition,
            signal->bitSize);
    return rawValue * signal->factor + signal->offset;
}

void translateCanSignal(USBDevice* usbDevice, CanSignal* signal, uint8_t* data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    float processedValue = handler(signal, signals, signalCount, value, &send);
    signal->lastValue = value;

    if(send) {
        sendNumericalMessage(signal->genericName, processedValue, usbDevice);
    }
}

// TODO if we make value a void*, could this be used for all of the
// translateCanSignal functions? We can pass in the formats no problem, but the
// differently typed values mean you would need to repeat this entire function
// multiple files anyway. I'm leaving this function for now because it's useful
// in a custom handler.
void sendNumericalMessage(char* name, float value, USBDevice* usbDevice) {
    int messageLength = NUMERICAL_MESSAGE_FORMAT_LENGTH + strlen(name)
            + NUMERICAL_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, NUMERICAL_MESSAGE_FORMAT, name, value);

    sendMessage(usbDevice, (uint8_t*) message, strlen(message));
}

void translateCanSignal(USBDevice* usbDevice, CanSignal* signal, uint8_t* data,
        char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    char* stringValue = handler(signal, signals, signalCount, value, &send);
    signal->lastValue = value;

    if(send) {
        int messageLength = STRING_MESSAGE_FORMAT_LENGTH +
            strlen(signal->genericName) + STRING_MESSAGE_VALUE_MAX_LENGTH;
        char message[messageLength];
        sprintf(message, STRING_MESSAGE_FORMAT, signal->genericName, stringValue);

        sendMessage(usbDevice, (uint8_t*) message, strlen(message));
    }
}

void translateCanSignal(USBDevice* usbDevice, CanSignal* signal, uint8_t* data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    bool booleanValue = handler(signal, signals, signalCount, value, &send);
    signal->lastValue = value;

    if(send) {
        int messageLength = BOOLEAN_MESSAGE_FORMAT_LENGTH +
            strlen(signal->genericName) + BOOLEAN_MESSAGE_VALUE_MAX_LENGTH;
        char message[messageLength];
        sprintf(message, BOOLEAN_MESSAGE_FORMAT, signal->genericName,
                booleanValue ? "true" : "false");

        sendMessage(usbDevice, (uint8_t*) message, strlen(message));
    }
}

float passthroughHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    return value;
}

bool booleanHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    return value == 0.0 ? false : true;
}

float ignoreHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    *send = false;
    return 0.0;
}

char* stateHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    for(int i = 0; i < signal->stateCount; i++) {
        if(signal->states[i].value == value) {
            return signal->states[i].name;
        }
    }
    *send = false;
}

void translateCanSignal(USBDevice* usbDevice, CanSignal* signal, uint8_t* data,
        CanSignal* signals, int signalCount) {
    translateCanSignal(usbDevice, signal, data, passthroughHandler, signals,
            signalCount);
}

CanSignal* lookupSignal(char* name, CanSignal* signals, int signalCount) {
    for(int i = 0; i < signalCount; i++) {
        CanSignal* signal = &signals[i];
        if(!strcmp(name, signal->genericName)) {
            return signal;
        }
    }
    Serial.print("Could'nt find a signal with the genericName \:");
    Serial.print(name);
    Serial.println(" -- probably about to segfault");
    return 0;
}

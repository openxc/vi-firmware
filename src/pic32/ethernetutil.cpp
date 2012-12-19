#include "log.h"
#include "ethernetutil.h"

// This size can be set to any arbitrary value. Its
// function is just to define the size of the send
// buffer.
#define MAX_MESSAGE_SIZE 128


void initializeEthernet(EthernetDevice* device) {
    debug("Initializing Ethernet...");
#ifdef USE_DHCP
    Ethernet.begin();
#else
    Ethernet.begin(device->macAddress, device->ipAddress);
#endif
    QUEUE_INIT(uint8_t, &device->receiveQueue);
    QUEUE_INIT(uint8_t, &device->sendQueue);
    device->server.begin();
}

// The message bytes are sequentially popped from the
// send queue to the send buffer. After the buffer is full
// or the queue is empty, the contents of the buffer are
// sent over the ethernet to listening clients.
void processEthernetSendQueue(EthernetDevice* device) {
    unsigned int byteCount = 0;
    char sendBuffer[MAX_MESSAGE_SIZE];
    while(!QUEUE_EMPTY(uint8_t, &device->sendQueue) &&
            byteCount < MAX_MESSAGE_SIZE) {
        sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &device->sendQueue);
    }

    // must call at least one Ethernet method to keep the TCP/IP stack alive,
    // because it's implemented all in software - a quirk of the chipKIT
    // library. Ethernet.PeriodicTasks() is supposed to be specifically for this
    // purpose, but it doesn't seem to have any effect while this does.
    device->server.available();
    if(byteCount > 0) {
        device->server.write((uint8_t*) sendBuffer, byteCount);
    }
}

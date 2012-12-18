#include "log.h"
#include "ethernetutil.h"

void initializeEthernet(EthernetDevice* device, Server* server,
        uint8_t MACAddr[], uint8_t IPAddr[])
{
    debug("initializing Ethernet...");
    device->ptrServer = server;
    Ethernet.begin(MACAddr, IPAddr);
    device->ptrServer->begin();
}

// The message bytes are sequentially popped from the
// send queue to the send buffer. After the buffer is full
// or the queue is emtpy, the contents of the buffer are
// sent over the ethernet to listening clients.
void processEthernetSendQueue(EthernetDevice* device)
{
    unsigned int byteCount = 0;
    char sendBuffer[MAX_MESSAGE_SIZE];
    while(!QUEUE_EMPTY(uint8_t, &device->sendQueue) &&
            byteCount < MAX_MESSAGE_SIZE) {
        sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &device->sendQueue);
    }

    device->ptrServer->write((uint8_t*) sendBuffer, byteCount);
}

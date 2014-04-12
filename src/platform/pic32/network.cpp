#include "interface/network.h"
#include "util/log.h"
#include "util/bytebuffer.h"
#include <stddef.h>

#ifdef __USE_NETWORK__

// This size can be set to any arbitrary value. Its
// function is just to define the size of the send
// buffer.
#define MAX_MESSAGE_SIZE 128
#define DEFAULT_NETWORK_PORT 1776
#define DEFAULT_MAC_ADDRESS {0, 0, 0, 0, 0, 0}
#define DEFAULT_IP_ADDRESS {192, 168, 1, 100}

using openxc::util::log::debug;
using openxc::util::bytebuffer::processQueue;

Server server = Server(DEFAULT_NETWORK_PORT);

void openxc::interface::network::initialize(NetworkDevice* device) {
    if(device != NULL) {
        network::initializeCommon(device);
        device->macAddress = DEFAULT_MAC_ADDRESS;
        device->ipAddress = DEFAULT_IP_ADDRESS;
        device->server = &server;
#ifdef USE_DHCP
        server.begin();
#else
        server.begin(device->macAddress, device->ipAddress);
#endif
        device->server->begin();
        debug("Done.");
    }
}

// The message bytes are sequentially popped from the
// send queue to the send buffer. After the buffer is full
// or the queue is empty, the contents of the buffer are
// sent over the network to listening clients.
void openxc::interface::network::processSendQueue(NetworkDevice* device) {
    unsigned int byteCount = 0;
    char sendBuffer[MAX_MESSAGE_SIZE];
    while(!QUEUE_EMPTY(uint8_t, &device->sendQueue) &&
            byteCount < MAX_MESSAGE_SIZE) {
        sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &device->sendQueue);
    }

    // must call at least one Network method to keep the TCP/IP stack alive,
    // because it's implemented all in software - a quirk of the chipKIT
    // library. Network.PeriodicTasks() is supposed to be specifically for this
    // purpose, but it doesn't seem to have any effect while this does.
    device->server->available();
    if(byteCount > 0) {
        device->server->write((uint8_t*) sendBuffer, byteCount);
    }
}

void openxc::interface::network::read(NetworkDevice* device,
        IncomingMessageCallback callback) {
    Client client = device->server->available();
    if(client) {
        uint8_t byte;
        while((byte = client.read()) != -1 &&
                !QUEUE_FULL(uint8_t, &device->receiveQueue)) {
            QUEUE_PUSH(uint8_t, &device->receiveQueue, byte);
        }
        processQueue(&device->receiveQueue, callback);
    }
}

#else

void openxc::interface::network::read(NetworkDevice* device,
        openxc::util::bytebuffer::IncomingMessageCallback callback) { }
void openxc::interface::network::initialize(NetworkDevice* device) { }
void openxc::interface::network::processSendQueue(NetworkDevice* device) { }

#endif

// Structures and functions for ethernet transfer of
// CAN messages.
//
#ifndef _ETHERNETUTIL_H_
#define _ETHERNETUTIL_H_

#ifdef __PIC32__
#include "chipKITEthernet.h"
#endif // __PIC32__

#ifdef __cplusplus
extern "C" {
#endif

#include "queue.h"

// This size can be set to any arbitrary value. Its
// function is just to define the size of the send
// buffer.
#define MAX_MESSAGE_SIZE 128

// Public: A container for an ethernet connection with queues for both input and
// output.
//
// sendQueue - A queue of bytes that need to be sent out over ethernet.
// receiveQueue - A queue of bytes that have been received via ethernet but not yet
// 		processed.
// ptrServer - A pointer to the ethernet server which will
// send CAN data to connected clients.
//
typedef struct {
	Server* ptrServer;

    // device to host
    ByteQueue sendQueue;
    // host to device
    ByteQueue receiveQueue;
} EthernetDevice;

// Initializes the ethernet interface with MAC and IP addresses, starts
// listening for connections.
void initializeEthernet(EthernetDevice* device, Server* server,
		uint8_t MACAddr[], uint8_t IPAddr[]);

// Processes the ethernet send queue and sends its bytes to
// connected ethernet clients.
void processEthernetSendQueue(EthernetDevice* device);



#ifdef __cplusplus
}
#endif

#endif

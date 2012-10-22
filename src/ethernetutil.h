#ifndef _ETHERNETUTIL_H_
#define _ETHERNETUTIL_H_

#ifdef __PIC32__
#include "chipKITEthernet.h"
#endif // __PIC32__

#ifdef __cplusplus
extern "C" {
#endif

#include "queue.h"

#define MAX_MESSAGE_SIZE 128

typedef struct {
	Server* ptrServer;

    // device to host
    ByteQueue sendQueue;
    // host to device
    ByteQueue receiveQueue;
} EthernetDevice;

void initializeEthernet(EthernetDevice* device, Server* server,
		uint8_t MACAddr[], uint8_t IPAddr[]);
void processEthernetSendQueue(EthernetDevice* device);



#ifdef __cplusplus
}
#endif

#endif

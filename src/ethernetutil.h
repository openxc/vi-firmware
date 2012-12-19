#ifndef _ETHERNETUTIL_H_
#define _ETHERNETUTIL_H_

#ifdef __PIC32__
#include "chipKITEthernet.h"
#endif // __PIC32__

#ifdef __cplusplus
extern "C" {
#endif

#define USE_DHCP

#include "queue.h"

/* Public: A container for an ethernet connection with queues for both input and
 * output.
 *
 * server - An instance of Server which will allow connections from network
 *      clients.
 * macAddress - MAC address for network device. DEFAULT_MAC_ADRESS will use the
 *      hardware-specified address.
 * ipAddress - static IP address for the network device. If USE_DHCP is defined,
 *      this is ignored.
 *
 * sendQueue - A queue of bytes that need to be sent out over ethernet.
 * receiveQueue - A queue of bytes that have been received via ethernet but not yet
 *      processed.
 */
typedef struct {
    uint8_t ipAddress[4];
    uint8_t macAddress[6];

    // device to host
    ByteQueue sendQueue;
    // host to device
    ByteQueue receiveQueue;
#ifdef __PIC32__
    Server* server;
#endif // __PIC32__
} EthernetDevice;

/* Initializes the ethernet interface with MAC and IP addresses, starts
 * listening for connections.
 */
void initializeEthernet(EthernetDevice* device);

/* Processes the ethernet send queue and sends its bytes to connected ethernet
 * clients.
 */
void processEthernetSendQueue(EthernetDevice* device);

#ifdef __cplusplus
}
#endif

#endif

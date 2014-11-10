#ifndef __INTERFACE_NETWORK_H__
#define __INTERFACE_NETWORK_H__

#include <stdlib.h>

#if defined(__PIC32__) && defined(__USE_NETWORK__)
#include "chipKITEthernet.h"
#endif // __USE_NETWORK__

#include "util/bytebuffer.h"
#include "commands/commands.h"
#include "interface.h"

#define USE_DHCP

namespace openxc {
namespace interface {
namespace network {

/* Public: A container for an network connection with queues for both input and
 * output.
 *
 * descriptor - A general descriptor for this interface.
 * macAddress - MAC address for network device. DEFAULT_MAC_ADRESS will use the
 *      hardware-specified address.
 * ipAddress - static IP address for the network device. If USE_DHCP is defined,
 *      this is ignored.
 *
 * sendQueue - A queue of bytes that need to be sent out over an IP network.
 * receiveQueue - A queue of bytes that have been received via an IP network but
 *      not yet processed.
 * server - An instance of Server which will allow connections from network
 *      clients.
 */
typedef struct {
    InterfaceDescriptor descriptor;
    uint8_t ipAddress[4];
    uint8_t macAddress[6];
    bool configured;

    // device to host
    QUEUE_TYPE(uint8_t) sendQueue;
    // host to device
    QUEUE_TYPE(uint8_t) receiveQueue;
#if defined(__PIC32__) && defined(__USE_NETWORK__)
    Server* server;
#endif // __USE_NETWORK__
} NetworkDevice;

/* Public: Perform platform-agnostic Network initialization.
 */
void initializeCommon(NetworkDevice* device);

/* Initializes the network interface with MAC and IP addresses, starts
 * listening for connections.
 */
void initialize(NetworkDevice* device);

/* Processes the network send queue and sends its bytes to connected network
 * clients.
 */
void processSendQueue(NetworkDevice* device);

void read(NetworkDevice* device,
        openxc::util::bytebuffer::IncomingMessageCallback callback);

size_t handleIncomingMessage(uint8_t payload[], size_t length);

/* Public: Check the connection status of a network device.
 *
 * Returns true if a network client is connected.
 */
bool connected(NetworkDevice* device);

} // namespace network
} // namespace interface
} // namespace openxc

#endif // __INTERFACE_NETWORK_H__

#ifndef __INTERFACE_BLE_H__
#define __INTERFACE_BLE_H__

#include <stdlib.h>
#include "interface/interface.h"
#include "util/bytebuffer.h"


namespace openxc {
namespace interface {
namespace ble {

/* Public: A container for an network connection with queues for both input and
 * output.
 *
 * descriptor - A general descriptor for this interface.
 * sendQueue - A queue of bytes that need to be sent out over an IP network.
 * receiveQueue - A queue of bytes that have been received via an IP network but
 *      not yet processed.
 */


typedef struct {
    const char * advname;
    uint16_t adv_min_ms;
    uint16_t adv_max_ms;
    uint16_t slave_min_ms; 
    uint16_t slave_max_ms;
    uint8_t  bdaddr[6];
}BleSettings;

typedef enum {
    SET_CONNECTABLE_FAILED = 128,
    ISR_SPI_READ_TIMEDOUT  = 129,
    DISCONNECT_DEVICE_FAILED = 130,
    MULTIPLE_DEVICES_CONNECTED = 131,
} BleError;

typedef enum {
    RADIO_OFF   = 0,
    RADIO_ON_NOT_ADVERTISING = 1,
    ADVERTISING = 2,
    CONNECTED   = 3,
    NOTIFICATION_ENABLED = 4,
} BleStatus;

typedef struct {
    InterfaceDescriptor descriptor;
    BleSettings         blesettings;
    QUEUE_TYPE(uint8_t) sendQueue;
    QUEUE_TYPE(uint8_t) receiveQueue;
    bool configured;
    BleStatus status;
} BleDevice;


/* Public: Perform platform-agnostic Network initialization.
 */
void initializeCommon(BleDevice* device);

/* Initializes the network interface with MAC and IP addresses, starts
 * listening for connections.
 */
bool initialize(BleDevice* device);

/* Processes the network send queue and sends its bytes to connected network
 * clients.
 */
void processSendQueue(BleDevice* device);

void read(BleDevice* device);

size_t handleIncomingMessage(uint8_t payload[], size_t length);

/* Public: Check the connection status of a network device.
 *
 * Returns true if a ble hardware is connected.
 */
bool connected(BleDevice* device);


void deinitialize(BleDevice* device);

void deinitializeCommon(BleDevice* device);

} // namespace ble
} // namespace interface
} // namespace openxc

#endif // __INTERFACE_BLE_H__

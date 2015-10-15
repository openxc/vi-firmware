#ifndef _LISTENER_H_
#define _LISTENER_H_

#include "interface/usb.h"
#include "interface/uart.h"
#include "interface/network.h"
#include "platform/pic32/telit_he910.h"
#include "platform/pic32/telit_he910_platforms.h"

using openxc::interface::uart::UartDevice;
using openxc::interface::usb::UsbDevice;
using openxc::interface::network::NetworkDevice;
using openxc::telitHE910::TelitDevice;

#define MAX_OUTGOING_PAYLOAD_SIZE 256

namespace openxc {
namespace pipeline {

typedef enum {
    SIMPLE,
    CAN,
    DIAGNOSTIC,
    LOG,
    COMMAND_RESPONSE,
} MessageClass;

/* Public: A container for all output devices that want to be notified of new
 *      messages from the CAN bus.
 *
 * This structure sets up a standard interface for all output devices to receive
 * updates from CAN. Right now, this means USB and UART, but it can be extended
 * to output over another UART, Network, WiFi, etc.
 *
 * TODO This file could most likely be refactored and improved. Ideally these
 * output interfaces would all have the same type, so this could just be a list
 * of "receiver" functions. maybe instead of the devices, this is a list of the
 * sendQueues?
 */
typedef struct {
    UsbDevice* usb;
    UartDevice* uart;
    TelitDevice* telit;
    NetworkDevice* network;
} Pipeline;

/* Public: Serialize the message to a bytestream (conforming to the OpenXC
 * standard and the currently selected payload format) and send it out to the
 * pipeline.
 *
 * This will accept both raw and translated typed messages.
 *
 * message - A message structure containing the type and data for the message.
 * pipeline - The pipeline to send on.
 */
void publish(openxc_VehicleMessage* message,
        openxc::pipeline::Pipeline* pipeline);

/* Public: Queue the message to send on all of the interfaces registered with
 *      the pipeline. If the any of the queues does not have sufficient capacity
 *      to store the message, it will be dropped for that interface only (i.e.
 *      UART can be overloaded and dropping messages but USB will continue
 *      with a 100% translation rate).
 *
 * pipeline - Container of all pipelines to send the message on.
 * message - The message data as an array of uint8_t.
 * messageSize - The length of the message's byte array.
 * messageClass - the class of the message, used to decide which endpoints in
 *      the pipeline receive the message.
 */
void sendMessage(Pipeline* pipeline, uint8_t* message, int messageSize,
        MessageClass messageClass);

/* Public: Perform interface-specific functions to flush all message queues out
 *      to their respective physical interfaces.
 *
 * TODO This is the tricky part with making the pipeline more generic - this
 * needs to call an interface-specific method for each queue.
 *
 * pipeline - Pipeline instance with the interface queues to flush.
 */
void process(Pipeline* pipeline);

void logStatistics(Pipeline* pipeline);

} // namespace interface
} // namespace openxc

#endif // _LISTENER_H_

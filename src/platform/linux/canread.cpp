#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <linux/can.h>
#include "can/canutil.h"
#include "signals.h"
#include "util/log.h"
#include "platform/linux/canread.h"

using openxc::util::log::debug;
using openxc::signals::getCanBusCount;
using openxc::signals::getCanBuses;
using openxc::can::shouldAcceptMessage;

void printCanMessage(CanMessage* message) {
	printf("Id: %4x, length: %d, data:", message->id, message->length);
	for (int i = 0; i < message->length; i++) {
		printf("%2x ", message->data[i]);
	}
	printf("\n");
}

CanMessage* receiveCanMessage(int socketId) {
	struct can_frame frame;
	int nbytes = read(socketId, &frame, sizeof(struct can_frame));
	if (nbytes < 0) {
		std::cout << "Unable to read frame.\n";
	} else if (nbytes < sizeof(struct can_frame)) {
        	std::cout << "Incomplete CAN frame.\n";
       	} else {
		std::cout << "New CAN message received.\n";

    		CanMessage result = {
        		id: frame.can_id,
        		format: CanMessageFormat::STANDARD,
        		data: {0},
        		length: frame.can_dlc
    		};

		memcpy(result.data, frame.data, frame.can_dlc);
//		printCanMessage(&result);
    		return &result;
	}
	return NULL;
}

void canReaderHandler(CanBus* bus, int socketId) {
	while (1) {
		CanMessage* message = receiveCanMessage(socketId);
            	if (message != NULL && shouldAcceptMessage(bus, message->id) &&
                		!QUEUE_PUSH(CanMessage, &bus->receiveQueue, *message)) {
                	// An exception to the "don't leave commented out code" rule,
                	// this log statement is useful for debugging performance issues
                	// but if left enabled all of the time, it can can slown down
                	// the interrupt handler so much that it locks up the device in
                	// permanent interrupt handling land.
                	//
                	// debug("Dropped CAN message with ID 0x%02x -- queue is full",
                	// message.id);
			std::cout << "Dropped CAN message.\n";
                	++bus->messagesDropped;
            	}
        }
}

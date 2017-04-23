#include <iostream>
#include "interface/network.h"

void openxc::interface::network::initialize(NetworkDevice* uart) { 
	std::cout << "Network initialized\n";
}

void openxc::interface::network::processSendQueue(NetworkDevice* device) { 
	std::cout << "New message to Network:\n";
   	while(!QUEUE_EMPTY(uint8_t, &device->sendQueue)) {
        	uint8_t byte = QUEUE_PEEK(uint8_t, &device->sendQueue);
        	// We used to use non-blocking here, but then we got into a race
        	// condition - if the transmit interrupt occurred while adding more data
        	// to the queue, you could lose data. We should be able to switch back
        	// to non-blocking if we disabled interrupts while modifying the queue
        	// (good practice anyway) but for now switching this to block sends
        	// seems to work OK without any significant impacts.
		std::cout << byte;
   		QUEUE_POP(uint8_t, &device->sendQueue);
    	}
	std::cout << "\n";
}

void openxc::interface::network::read(NetworkDevice* device, openxc::util::bytebuffer::IncomingMessageCallback callback) { }

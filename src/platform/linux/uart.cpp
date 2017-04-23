#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "interface/uart.h"
#include "pipeline.h"
#include "config.h"
#include "util/bytebuffer.h"
#include "util/log.h"
#include "gpio.h"

using openxc::config::getConfiguration;
using openxc::util::log::debug;
using openxc::pipeline::Pipeline;
using openxc::util::bytebuffer::processQueue;
using openxc::gpio::GpioValue;
using openxc::gpio::GpioDirection;

/* 
 * Disable request to send through RTS line. We cannot handle any more data
 * right now.
 */
void pauseReceive() {
}

/*
 * Enable request to send through RTS line. We can handle more data now. 
 */
void resumeReceive() {
}

void handleTransmitInterrupt() {
	std::cout << "New message to UART:\n";
	uint8_t buffer[256];
	int noOfBytes = 0;
	while(!QUEUE_EMPTY(uint8_t, &getConfiguration()->uart.sendQueue)) {
        	uint8_t byte = QUEUE_PEEK(uint8_t, &getConfiguration()->uart.sendQueue);
        	// We used to use non-blocking here, but then we got into a race
        	// condition - if the transmit interrupt occurred while adding more data
        	// to the queue, you could lose data. We should be able to switch back
        	// to non-blocking if we disabled interrupts while modifying the queue
        	// (good practice anyway) but for now switching this to block sends
        	// seems to work OK without any significant impacts.
		std::cout << byte;
		buffer[noOfBytes++] = byte;
		QUEUE_POP(uint8_t, &getConfiguration()->uart.sendQueue);
    	}
	std::cout << "\n";
	write(getConfiguration()->uart.baudRate, buffer, noOfBytes - 1);
}

void openxc::interface::uart::read(UartDevice* device,
        openxc::util::bytebuffer::IncomingMessageCallback callback) {
    if (device != NULL) {
        if(!QUEUE_EMPTY(uint8_t, &device->receiveQueue)) {
            processQueue(&device->receiveQueue, callback);
            if(!QUEUE_FULL(uint8_t, &device->receiveQueue)) {
                resumeReceive();
            }
        }
    }
}

void openxc::interface::uart::changeBaudRate(UartDevice* device, int baud) {
}

void openxc::interface::uart::writeByte(UartDevice* device, uint8_t byte) {
	std::cout << "UART Write: " << byte << "\n";
}

int openxc::interface::uart::readByte(UartDevice* device) {
}

void openxc::interface::uart::initialize(UartDevice* device) {
	int sockfd, portno;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     int n;

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        std::cout << "Socket::ERROR opening socket\n";

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = 8080;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              std::cout << "Scoket::ERROR on binding.\n";

     listen(sockfd, 5);
     clilen = sizeof(cli_addr);
     device->baudRate = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (device->baudRate < 0) 
          std::cout << "Socket::ERROR on accept.\n";

	std::cout << "UART Initialized\n";
}

void openxc::interface::uart::processSendQueue(UartDevice* device) {
	if(!QUEUE_EMPTY(uint8_t, &device->sendQueue)) {
		handleTransmitInterrupt();
	}
}

bool openxc::interface::uart::connected(UartDevice* device) {
	return device->baudRate > 0;
}


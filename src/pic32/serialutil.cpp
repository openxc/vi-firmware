/* UART interface for PIC32.
 *
 * On the PIC32, the CAN translator uses Serial from the chipKIT library's
 * HardwareSerial interface for vehicle data. This is equivalent to UART1A or
 * U1A. Hardware flow control (RTS/CTS) is enabled, so CTS must be pulled low
 * by the receiving device before data will be sent.
 *
 * UART1 is also used by the USB-Serial connection, so in order to flash the
 * PIC32, these Tx/Rx lines must be disconnected. Ideally we could leave that
 * UART interface for debugging, but there are conflicts with all other exposed
 * UART interfaces when using flow control.
 *
 * U1A, U2A and U3A all have CTS/RTS lines exposed on the chipKIT. U2A doesn't
 * have Rx/Tx exposed through the HardwareSerial library, so we avoid that to
 * minimize additional programming. U3A's CTS/RTS lines conflict with CAN1, so
 * that's out; that leaves U1A.
 *
 * Pin 0 - U1ARX, connect this to the TX line of the receiver.
 * Pin 1 - U1ATX, connect this to the RX line of the receiver.
 * Pin 18 - U1ARTS, connect this to the CTS line of the receiver.
 * Pin 19 - U1ACTS, connect this to the RTS line of the receiver.
 */
#include "serialutil.h"
#include "buffers.h"
#include "log.h"
#include "HardwareSerial.h"
#include "WProgram.h"

#define UART_BAUDRATE 460800
// See http://www.chipkit.org/forum/viewtopic.php?f=19&t=711
#define _UARTMODE_BRGH 3

// bit 8 in the uxMode register controls hardware flow control
#define _UARTMODE_FLOWCONTROL 8

extern HardwareSerial Serial;

// TODO see if we can do this with interrupts on the chipKIT
// http://www.chipkit.org/forum/viewtopic.php?f=7&t=1088
void readFromSerial(SerialDevice* device, bool (*callback)(uint8_t*)) {
    if(device != NULL) {
        int bytesAvailable = ((HardwareSerial*)device->controller)->available();
        if(bytesAvailable > 0) {
            for(int i = 0; i < bytesAvailable &&
                    !QUEUE_FULL(uint8_t, &device->receiveQueue); i++) {
                char byte = ((HardwareSerial*)device->controller)->read();
                QUEUE_PUSH(uint8_t, &device->receiveQueue, (uint8_t) byte);
            }
            processQueue(&device->receiveQueue, callback);
        }
    }
}

void initializeSerial(SerialDevice* device) {
    if(device != NULL) {
        initializeSerialCommon(device);
        device->controller = &Serial;
        ((HardwareSerial*)device->controller)->begin(UART_BAUDRATE);
        // Override baud rate setup to allow baud rates 200000 (see
        // http://www.chipkit.org/forum/viewtopic.php?f=19&t=711, this should
        // eventually make it into the MPIDE toolchain)
        ((p32_uart*)_UART1_BASE_ADDRESS)->uxBrg.reg = ((__PIC32_pbClk / 4 / UART_BAUDRATE) - 1);
        ((p32_uart*)_UART1_BASE_ADDRESS)->uxMode.reg = (1 << _UARTMODE_ON) | (1 << _UARTMODE_BRGH);
        // Manually enable RTS/CTS hardware flow control using the UART2
        // register. The chipKIT serial library doesn't have an interface to do
        // this, and the alternative UART libraries provided by Microchip are a
        // bit of a mess.
        ((p32_uart*)_UART1_BASE_ADDRESS)->uxMode.reg |= 2 << _UARTMODE_FLOWCONTROL;
        debug("Done.");
    }
}

// The chipKIT version of this function is blocking. It will entirely flush the
// send queue before returning.
void processSerialSendQueue(SerialDevice* device) {
    int byteCount = 0;
    char sendBuffer[MAX_MESSAGE_SIZE];
    while(!QUEUE_EMPTY(uint8_t, &device->sendQueue) &&
                    byteCount < MAX_MESSAGE_SIZE) {
        sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &device->sendQueue);
    }

    if(byteCount > 0) {
        ((HardwareSerial*)device->controller)->write((const uint8_t*)sendBuffer,
                byteCount);
    }
}

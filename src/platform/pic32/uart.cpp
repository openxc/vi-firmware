/* UART interface for PIC32.
 *
 * On the PIC32, the VI uses uart from the chipKIT library's
 * HardwareSerial interface for vehicle data. This is equivalent to UART1A or
 * U1A. Hardware flow control (RTS/CTS) is enabled, so CTS must be pulled low
 * by the receiving device before data will be sent.
 *
 * UART1 is also used by the USB-Uart connection, so in order to flash the
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
#include "interface/uart.h"
#include "util/bytebuffer.h"
#include "util/log.h"
#include "atcommander.h"
#include "WProgram.h"
#include "gpio.h"
#include "util/timer.h"

#if defined(CROSSCHASM_C5_BT)

    #define UART_STATUS_PORT 0
    #define UART_STATUS_PIN 58     // PORTB BIT4 (RB4)
    #define UART_STATUS_PIN_POLARITY 1    // high == connected

#elif defined(CROSSCHASM_C5_CELLULAR)

    #define UART_STATUS_PORT 0
    #define UART_STATUS_PIN 58     // PORTB BIT4 (RB4)
    #define UART_STATUS_PIN_POLARITY 1    // high == connected
    
#elif defined(CHIPKIT)

    #define UART_STATUS_PORT 0
    #define UART_STATUS_PIN A1

#endif

// See http://www.chipkit.org/forum/viewtopic.php?f=19&t=711
#define _UARTMODE_BRGH 3

// bit 8 in the uxMode register controls hardware flow control
#define _UARTMODE_FLOWCONTROL 8

namespace gpio = openxc::gpio;

using openxc::util::log::debug;
using openxc::util::bytebuffer::processQueue;
using openxc::util::time::uptimeMs;

extern const AtCommanderPlatform AT_PLATFORM_RN42;
extern HardwareSerial Serial;

// TODO see if we can do this with interrupts on the chipKIT
// http://www.chipkit.org/forum/viewtopic.php?f=7&t=1088
void openxc::interface::uart::read(UartDevice* device,
        openxc::util::bytebuffer::IncomingMessageCallback callback) {
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

/*TODO this is hard coded for UART1 anyway, so the argument is kind of
 * irrelevant.
 */
void openxc::interface::uart::changeBaudRate(UartDevice* device, int baud) {
    ((HardwareSerial*)device->controller)->begin(baud);
    // Override baud rate setup to allow baud rates 200000 (see
    // http://www.chipkit.org/forum/viewtopic.php?f=19&t=711, this should
    // eventually make it into the MPIDE toolchain)
    ((p32_uart*)_UART1_BASE_ADDRESS)->uxBrg.reg = ((__PIC32_pbClk / 4 /
            baud) - 1);
    ((p32_uart*)_UART1_BASE_ADDRESS)->uxMode.reg = (1 << _UARTMODE_ON) |
            (1 << _UARTMODE_BRGH);
    U1MODEbits.UEN = 2;
}

/* Private: Manually enable RTS/CTS hardware flow control using the UART2
 * register.
 *
 * The chipKIT uart library doesn't have an interface to do
 * this, and the alternative UART libraries provided by Microchip are a
 * bit of a mess.
 *
 * This is hard coded for UART1 anyway, so the 'device' argument is kind of
 * irrelevant.
 */
void enableFlowControl(openxc::interface::uart::UartDevice* device) {
    ((p32_uart*)_UART1_BASE_ADDRESS)->uxMode.reg |= 2 << _UARTMODE_FLOWCONTROL;
}

void openxc::interface::uart::writeByte(UartDevice* device, uint8_t byte) {
    ((HardwareSerial*)device->controller)->write(byte);
}

int openxc::interface::uart::readByte(UartDevice* device) {
    return ((HardwareSerial*)device->controller)->read();
}

void openxc::interface::uart::initialize(UartDevice* device) {
    if(device == NULL) {
        debug("Can't initialize a NULL UartDevice");
        return;
    }

    initializeCommon(device);
    device->controller = &Serial;
    changeBaudRate(device, device->baudRate);
    enableFlowControl(device);

    gpio::setDirection(UART_STATUS_PORT, UART_STATUS_PIN,
            gpio::GPIO_DIRECTION_INPUT);

    debug("Done.");
}

// The chipKIT version of this function is blocking. It will entirely flush the
// send queue before returning.
void openxc::interface::uart::processSendQueue(UartDevice* device) {
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

bool openxc::interface::uart::connected(UartDevice* device) {
    
    static unsigned int timer;
    bool status = false;
    static bool last_status = false;

#ifdef CHIPKIT

    // Use analogRead instead of digitalRead so we don't have to require
    // everyone *not* using UART to add an external pull-down resistor. When the
    // analog input is pulled down to GND, UART will be enabled.
    
    status = device != NULL && analogRead(UART_STATUS_PIN) < 100;

#else

    gpio::GpioValue value = gpio::getValue(UART_STATUS_PORT, UART_STATUS_PIN);
    switch(value) {
        case gpio::GPIO_VALUE_HIGH:
            status = UART_STATUS_PIN_POLARITY ? true : false;
            break;
        case gpio::GPIO_VALUE_LOW:
            status = UART_STATUS_PIN_POLARITY ? false : true;
            break;
         default:
            status = false;
            break;
    }

#endif

    // on apple devices, it takes some time for flow control to propagate to the device
    // during that time, if we transmit, we crash the connection
    // simple solution: rising delay for device connected status
    if(last_status == false && status == true) {
        timer = uptimeMs();
    }
    else if(last_status == true && status == true) {
        if(uptimeMs() - timer > 2500) {
            last_status = status;
            return true;
        } else {
            last_status = status;
            return false;
        }
    }
    
    last_status = status;
    
    return status;

}

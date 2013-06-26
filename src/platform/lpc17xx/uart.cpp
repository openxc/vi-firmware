#include <string.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "interface/uart.h"
#include "pipeline.h"
#include "atcommander.h"
#include "util/bytebuffer.h"
#include "util/log.h"
#include "util/timer.h"
#include "gpio.h"

// Only UART1 supports hardware flow control, so this has to be UART1
#define UART1_DEVICE (LPC_UART_TypeDef*)LPC_UART1

#define UART_STATUS_PORT 0
#define UART_STATUS_PIN 18

#ifdef BLUEBOARD

#define UART1_FUNCNUM 2
#define UART1_PORTNUM 2
#define UART1_TX_PINNUM 0
#define UART1_RX_PINNUM 1
#define UART1_FLOW_PORTNUM UART1_PORTNUM
#define UART1_FLOW_FUNCNUM UART1_FUNCNUM
#define UART1_CTS1_PINNUM 2
#define UART1_RTS1_PINNUM 7

#else

// Ford OpenXC CAN Translator Prototype
#define UART1_FUNCNUM 1
#define UART1_PORTNUM 0
#define UART1_TX_PINNUM 15
#define UART1_RX_PINNUM 16
#define UART1_FLOW_PORTNUM 2
#define UART1_FLOW_FUNCNUM 2
#define UART1_RTS1_PINNUM 2
#define UART1_CTS1_PINNUM 7

#endif

namespace gpio = openxc::gpio;

using openxc::util::time::delayMs;
using openxc::util::log::debugNoNewline;
using openxc::pipeline::Pipeline;
using openxc::util::bytebuffer::processQueue;
using openxc::gpio::GpioValue;
using openxc::gpio::GpioDirection;

extern const AtCommanderPlatform AT_PLATFORM_RN42;
extern Pipeline pipeline;

__IO int32_t RTS_STATE;
__IO FlagStatus TRANSMIT_INTERRUPT_STATUS;

/* Disable request to send through RTS line. We cannot handle any more data
 * right now.
 */
void pauseReceive() {
    if(RTS_STATE == ACTIVE) {
        // Disable request to send through RTS line
        UART_FullModemForcePinState(LPC_UART1, UART1_MODEM_PIN_RTS, INACTIVE);
        RTS_STATE = INACTIVE;
    }
}

/* Enable request to send through RTS line. We can handle more data now. */
void resumeReceive() {
    if (RTS_STATE == INACTIVE) {
        // Enable request to send through RTS line
        UART_FullModemForcePinState(LPC_UART1, UART1_MODEM_PIN_RTS, ACTIVE);
        RTS_STATE = ACTIVE;
    }
}

void disableTransmitInterrupt() {
    UART_IntConfig(UART1_DEVICE, UART_INTCFG_THRE, DISABLE);
}

void enableTransmitInterrupt() {
    TRANSMIT_INTERRUPT_STATUS = SET;
    UART_IntConfig(UART1_DEVICE, UART_INTCFG_THRE, ENABLE);
}

void handleReceiveInterrupt() {
    while(!QUEUE_FULL(uint8_t, &pipeline.uart->receiveQueue)) {
        uint8_t byte;
        uint32_t received = UART_Receive(UART1_DEVICE, &byte, 1, NONE_BLOCKING);
        if(received > 0) {
            QUEUE_PUSH(uint8_t, &pipeline.uart->receiveQueue, byte);
            if(QUEUE_FULL(uint8_t, &pipeline.uart->receiveQueue)) {
                pauseReceive();
            }
        } else {
            break;
        }
    }
}

void handleTransmitInterrupt() {
    disableTransmitInterrupt();

    while(UART_CheckBusy(UART1_DEVICE) == SET);

    while(!QUEUE_EMPTY(uint8_t, &pipeline.uart->sendQueue)) {
        uint8_t byte = QUEUE_PEEK(uint8_t, &pipeline.uart->sendQueue);
        if(UART_Send(UART1_DEVICE, &byte, 1, NONE_BLOCKING)) {
            QUEUE_POP(uint8_t, &pipeline.uart->sendQueue);
        } else {
            break;
        }
    }

    if(QUEUE_EMPTY(uint8_t, &pipeline.uart->sendQueue)) {
        disableTransmitInterrupt();
        TRANSMIT_INTERRUPT_STATUS = RESET;
    } else {
        enableTransmitInterrupt();
    }
}

extern "C" {

void UART1_IRQHandler() {
    uint32_t interruptSource = UART_GetIntId(UART1_DEVICE)
        & UART_IIR_INTID_MASK;
    switch(interruptSource) {
        case UART1_IIR_INTID_MODEM: {
            // Check Modem status
            uint8_t modemStatus = UART_FullModemGetStatus(LPC_UART1);
            // Check CTS status change flag
            if (modemStatus & UART1_MODEM_STAT_DELTA_CTS) {
                // if CTS status is active, continue to send data
                if (modemStatus & UART1_MODEM_STAT_CTS) {
                    UART_TxCmd(UART1_DEVICE, ENABLE);
                } else {
                    // Otherwise, Stop current transmission immediately
                    UART_TxCmd(UART1_DEVICE, DISABLE);
                }
            }
            break;
        }
        case UART_IIR_INTID_RDA:
        case UART_IIR_INTID_CTI:
            handleReceiveInterrupt();
            break;
        case UART_IIR_INTID_THRE:
            handleTransmitInterrupt();
            break;
        default:
            break;
    }
}

}

void openxc::interface::uart::read(UartDevice* device, bool (*callback)(uint8_t*)) {
    if(device != NULL) {
        if(!QUEUE_EMPTY(uint8_t, &device->receiveQueue)) {
            processQueue(&device->receiveQueue, callback);
            if(!QUEUE_FULL(uint8_t, &device->receiveQueue)) {
                resumeReceive();
            }
        }
    }
}

/* Auto flow control does work, but it turns the uart write functions into
 * blocking functions, which drags USB down. Instead we handle it manually so we
 * can make them asynchronous and let USB run at full speed.
 */
void configureFlowControl() {
    if (UART_FullModemGetStatus(LPC_UART1) & UART1_MODEM_STAT_CTS) {
        // Enable UART Transmit
        UART_TxCmd(UART1_DEVICE, ENABLE);
    }

    // Enable Modem status interrupt
    UART_IntConfig(UART1_DEVICE, UART1_INTCFG_MS, ENABLE);
    // Enable CTS1 signal transition interrupt
    UART_IntConfig(UART1_DEVICE, UART1_INTCFG_CTS, ENABLE);

    // TODO why is this neccessary? what aren't we doing correctly with flow
    // controlled receive?
    UART_FullModemConfigMode(LPC_UART1, UART1_MODEM_MODE_AUTO_RTS, ENABLE);
}

void configureUartPins() {
    PINSEL_CFG_Type PinCfg;

    PinCfg.Funcnum = UART1_FUNCNUM;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = UART1_PORTNUM;
    PinCfg.Pinnum = UART1_TX_PINNUM;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = UART1_RX_PINNUM;
    PINSEL_ConfigPin(&PinCfg);

    PinCfg.Portnum = UART1_FLOW_PORTNUM;
    PinCfg.Funcnum = UART1_FLOW_FUNCNUM;
    PinCfg.Pinnum = UART1_CTS1_PINNUM;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = UART1_RTS1_PINNUM;
    PINSEL_ConfigPin(&PinCfg);
}

void configureFifo() {
    UART_FIFO_CFG_Type fifoConfig;
    UART_FIFOConfigStructInit(&fifoConfig);
    UART_FIFOConfig(UART1_DEVICE, &fifoConfig);
}

void configureInterrupts() {
    UART_IntConfig(UART1_DEVICE, UART_INTCFG_RBR, ENABLE);
    enableTransmitInterrupt();
    /* preemption = 1, sub-priority = 1 */
    NVIC_SetPriority(UART1_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(UART1_IRQn);
}

void configureUart(int baud) {
    UART_CFG_Type UARTConfigStruct;
    UART_ConfigStructInit(&UARTConfigStruct);
    UARTConfigStruct.Baud_rate = baud;
    UART_Init(UART1_DEVICE, &UARTConfigStruct);

    configureFifo();
    configureInterrupts();
    configureFlowControl();

    TRANSMIT_INTERRUPT_STATUS = RESET;
}

void writeByte(uint8_t byte) {
    UART_SendByte(UART1_DEVICE, byte);
}

int readByte() {
    if(!QUEUE_EMPTY(uint8_t, &pipeline.uart->receiveQueue)) {
        return QUEUE_POP(uint8_t, &pipeline.uart->receiveQueue);
    }
    return -1;
}

// TODO this is stupid, fix the at-commander API
void delay(long unsigned int delayInMs) {
    delayMs(delayInMs);
}

void openxc::interface::uart::initialize(UartDevice* device) {
    if(device == NULL) {
        debug("Can't initialize a NULL UartDevice");
        return;
    }
    initializeCommon(device);

    // Configure P0.18 as an input, pulldown
    LPC_PINCON->PINMODE1 |= (1 << 5);
    // Ensure BT reset line is held high.
    LPC_GPIO1->FIODIR |= (1 << 17);

    gpio::setDirection(UART_STATUS_PORT, UART_STATUS_PIN,
            GpioDirection::GPIO_DIRECTION_INPUT);

    AtCommanderConfig config = {AT_PLATFORM_RN42};

    config.baud_rate_initializer = configureUart;
    config.write_function = writeByte;
    config.read_function = readByte;
    config.delay_function = delay;
    config.log_function = debugNoNewline;

    configureUartPins();

    if(at_commander_set_baud(&config, BAUD_RATE)) {
        debug("Successfully set baud rate");
        at_commander_reboot(&config);
    } else {
        debug("Unable to set baud rate of attached UART device");
    }

    // TODO
    QUEUE_INIT(uint8_t, &device->receiveQueue);
    UART_FullModemConfigMode(LPC_UART1, UART1_MODEM_MODE_AUTO_RTS, DISABLE);

    debug("Done.");
}

void openxc::interface::uart::processSendQueue(UartDevice* device) {
    if(!QUEUE_EMPTY(uint8_t, &device->sendQueue)) {
        if(TRANSMIT_INTERRUPT_STATUS == RESET) {
            handleTransmitInterrupt();
        } else {
            enableTransmitInterrupt();
        }
    }
}

bool openxc::interface::uart::connected(UartDevice* device) {
    return device != NULL && gpio::getValue(UART_STATUS_PORT, UART_STATUS_PIN)
            != GpioValue::GPIO_VALUE_LOW;
}


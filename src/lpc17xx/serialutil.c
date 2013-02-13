#include <string.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "serialutil.h"
#include "listener.h"
#include "buffers.h"
#include "log.h"

// Only UART1 supports hardware flow control, so this has to be UART1
#define UART1_DEVICE (LPC_UART_TypeDef*)LPC_UART1
#define UART_BAUDRATE 230000

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

extern Listener listener;

__IO int32_t RTS_STATE;
__IO int32_t CTS_STATE;
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
    TRANSMIT_INTERRUPT_STATUS = RESET;
}

void enableTransmitInterrupt() {
    if(TRANSMIT_INTERRUPT_STATUS == RESET) {
        UART_IntConfig(UART1_DEVICE, UART_INTCFG_THRE, ENABLE);
        TRANSMIT_INTERRUPT_STATUS = SET;
    }
}

void handleReceiveInterrupt() {
    while(!QUEUE_FULL(uint8_t, &listener.serial->receiveQueue)) {
        uint8_t byte;
        uint32_t received = UART_Receive(UART1_DEVICE, &byte, 1, NONE_BLOCKING);
        if(received > 0) {
            QUEUE_PUSH(uint8_t, &listener.serial->receiveQueue, byte);
            if(QUEUE_FULL(uint8_t, &listener.serial->receiveQueue)) {
                pauseReceive();
            }
        } else {
            break;
        }
    }
}

void handleTransmitInterrupt() {
    disableTransmitInterrupt();

    if(CTS_STATE == INACTIVE) {
        return;
    }

    while(UART_CheckBusy(UART1_DEVICE) == SET);

    while(!QUEUE_EMPTY(uint8_t, &listener.serial->sendQueue)) {
        uint8_t byte = QUEUE_PEEK(uint8_t, &listener.serial->sendQueue);
        if(UART_Send(UART1_DEVICE, &byte, 1, NONE_BLOCKING)) {
            QUEUE_POP(uint8_t, &listener.serial->sendQueue);
        } else {
            break;
        }
    }

    if(QUEUE_EMPTY(uint8_t, &listener.serial->sendQueue)) {
        disableTransmitInterrupt();
    } else {
        enableTransmitInterrupt();
    }
}

void UART1_IRQHandler() {
    uint32_t interruptSource = UART_GetIntId(UART1_DEVICE)
        & UART_IIR_INTID_MASK;
    if(interruptSource == 0) {
        // Check Modem status
        uint8_t modemStatus = UART_FullModemGetStatus(LPC_UART1);
        // Check CTS status change flag
        if (modemStatus & UART1_MODEM_STAT_DELTA_CTS) {
            // if CTS status is active, continue to send data
            if (modemStatus & UART1_MODEM_STAT_CTS) {
                CTS_STATE = ACTIVE;
                UART_TxCmd(UART1_DEVICE, ENABLE);
            } else {
                // Otherwise, Stop current transmission immediately
                CTS_STATE = INACTIVE;
                UART_TxCmd(UART1_DEVICE, DISABLE);
            }
        }
    }

    switch(interruptSource) {
        case UART_IIR_INTID_RDA:
        case UART_IIR_INTID_CTI:
            handleReceiveInterrupt();
            break;
        case UART_IIR_INTID_THRE:
            handleTransmitInterrupt();
            break;
    }
}

void readFromSerial(SerialDevice* device, bool (*callback)(uint8_t*)) {
    if(device != NULL) {
        if(!QUEUE_EMPTY(uint8_t, &device->receiveQueue)) {
            processQueue(&device->receiveQueue, callback);
            if(!QUEUE_FULL(uint8_t, &device->receiveQueue)) {
                resumeReceive();
            }
        }
    }
}

/* Auto flow control does work, but it turns the serial write functions into
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
    resumeReceive();
}

void configurePins() {
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

void configureUart() {
    UART_CFG_Type UARTConfigStruct;
    UART_ConfigStructInit(&UARTConfigStruct);
    UARTConfigStruct.Baud_rate = UART_BAUDRATE;
    UART_Init(UART1_DEVICE, &UARTConfigStruct);
}

void configureInterrupts() {
    UART_IntConfig(UART1_DEVICE, UART_INTCFG_RBR, ENABLE);
    UART_IntConfig(UART1_DEVICE, UART_INTCFG_RLS, ENABLE);
    enableTransmitInterrupt();
    /* preemption = 1, sub-priority = 1 */
    NVIC_SetPriority(UART1_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(UART1_IRQn);
}

void initializeSerial(SerialDevice* device) {
    if(device != NULL) {
        initializeSerialCommon(device);

        configurePins();
        configureUart();
        configureFifo();
        configureFlowControl();
        configureInterrupts();
        CTS_STATE = ACTIVE;

        // Configure P0.18 as an input, pulldown
        LPC_PINCON->PINMODE1 |= (1 << 5);
        // Ensure BT reset line is held high.
        LPC_GPIO1->FIODIR |= (1 << 17);
        LPC_GPIO1->FIOPIN |= (1 << 17);
        debug("Done.");
    }
}

void processSerialSendQueue(SerialDevice* device) {
    if(!QUEUE_EMPTY(uint8_t, &device->sendQueue)) {
        enableTransmitInterrupt();
        handleTransmitInterrupt();
    }
}

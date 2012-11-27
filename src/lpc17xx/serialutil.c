#include <string.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "serialutil.h"
#include "buffers.h"
#include "log.h"

#define CAN_SERIAL_PORT (LPC_UART_TypeDef*)LPC_UART1


#ifdef BLUEBOARD

#define UART1_FUNCNUM 2
#define UART1_PORTNUM 2
#define UART1_TX_PINNUM 0
#define UART1_RX_PINNUM 1
#define UART1_CTS1_PINNUM 2
#define UART1_RTS1_PINNUM 7

#else

// Ford OpenXC CAN Translator Prototype
#define UART1_FUNCNUM 1
#define UART1_PORTNUM 0
#define UART1_TX_PINNUM 15
#define UART1_RX_PINNUM 16
// TODO these are incorrect because the layout doesn't connect the pins yet
#define UART1_CTS1_PINNUM 2
#define UART1_RTS1_PINNUM 7

#endif

extern SerialDevice SERIAL_DEVICE;

__IO FlagStatus TRANSMIT_INTERRUPT_STATUS;

void handleReceiveInterrupt() {
    while(!QUEUE_FULL(uint8_t, &SERIAL_DEVICE.receiveQueue)) {
        uint8_t byte;
        uint32_t bytesReceived = UART_Receive(CAN_SERIAL_PORT,
                &byte, 1, NONE_BLOCKING);
        if(bytesReceived > 0) {
            QUEUE_PUSH(uint8_t, &SERIAL_DEVICE.receiveQueue, byte);
        } else {
            break;
        }
    }
}

void handleTransmitInterrupt() {
    UART_IntConfig(CAN_SERIAL_PORT, UART_INTCFG_THRE, DISABLE);

    /* Wait for FIFO buffer empty, transfer UART_TX_FIFO_SIZE bytes
     * of data or break whenever ring buffers are empty */
    /* Wait until THR empty */
    while(UART_CheckBusy(CAN_SERIAL_PORT) == SET);

    while(!QUEUE_EMPTY(uint8_t, &SERIAL_DEVICE.sendQueue)) {
        uint8_t byte = QUEUE_PEEK(uint8_t, &SERIAL_DEVICE.sendQueue);
        if(UART_Send(CAN_SERIAL_PORT, &byte, 1, NONE_BLOCKING)) {
            QUEUE_POP(uint8_t, &SERIAL_DEVICE.sendQueue);
        } else {
            break;
        }
    }

    if(QUEUE_EMPTY(uint8_t, &SERIAL_DEVICE.sendQueue)) {
        UART_IntConfig(CAN_SERIAL_PORT, UART_INTCFG_THRE, DISABLE);
        TRANSMIT_INTERRUPT_STATUS = RESET;
    } else {
        UART_IntConfig(CAN_SERIAL_PORT, UART_INTCFG_THRE, ENABLE);
        TRANSMIT_INTERRUPT_STATUS = SET;
    }
}

void UART1_IRQHandler() {
    uint32_t interruptSource = UART_GetIntId(CAN_SERIAL_PORT)
        & UART_IIR_INTID_MASK;

    switch(interruptSource) {
        case UART_IIR_INTID_RLS:
            break;
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
    if(!QUEUE_EMPTY(uint8_t, &device->receiveQueue)) {
        processQueue(&device->receiveQueue, callback);
    }
}

void initializeSerial(SerialDevice* device) {
    QUEUE_INIT(uint8_t, &device->receiveQueue);
    QUEUE_INIT(uint8_t, &device->sendQueue);

    UART_CFG_Type UARTConfigStruct;
    PINSEL_CFG_Type PinCfg;

    PinCfg.Funcnum = UART1_FUNCNUM;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = UART1_PORTNUM;
    PinCfg.Pinnum = UART1_TX_PINNUM;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = UART1_RX_PINNUM;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = UART1_CTS1_PINNUM;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = UART1_RTS1_PINNUM;
    PINSEL_ConfigPin(&PinCfg);

    TRANSMIT_INTERRUPT_STATUS = RESET;

    UART_ConfigStructInit(&UARTConfigStruct);
    UARTConfigStruct.Baud_rate = 921600;
    UART_Init(CAN_SERIAL_PORT, &UARTConfigStruct);

    UART_FIFO_CFG_Type fifoConfig;
    UART_FIFOConfigStructInit(&fifoConfig);
    UART_FIFOConfig(CAN_SERIAL_PORT, &fifoConfig);

    // Configure hardware flow control
    UART_FullModemForcePinState((LPC_UART1_TypeDef*)CAN_SERIAL_PORT,
            UART1_MODEM_PIN_RTS, ACTIVE);

    UART_TxCmd(CAN_SERIAL_PORT, ENABLE);

    UART_IntConfig(CAN_SERIAL_PORT, UART_INTCFG_RBR, ENABLE);
    NVIC_SetPriority(UART1_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(UART1_IRQn);
}

void processSerialSendQueue(SerialDevice* device) {
    if(!QUEUE_EMPTY(uint8_t, &device->sendQueue)) {
        handleTransmitInterrupt();
        if(TRANSMIT_INTERRUPT_STATUS == RESET) {
            handleTransmitInterrupt();
        } else {
            UART_IntConfig(CAN_SERIAL_PORT, UART_INTCFG_THRE, ENABLE);
        }
    }
}

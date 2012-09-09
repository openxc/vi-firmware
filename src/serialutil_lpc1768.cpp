#ifdef __LPC17XX__

#include <string.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "serialutil.h"
#include "buffers.h"
#include "log.h"

#define CAN_SERIAL_PORT (LPC_UART_TypeDef*)LPC_UART1

extern SerialDevice SERIAL_DEVICE;

extern "C" {

int bufferIndex = 0;
char buffer[MAX_MESSAGE_SIZE];

void UART1_IRQHandler() {
    if((UART_GetIntId((LPC_UART_TypeDef*)LPC_UART1) & UART_IIR_INTID_MASK) == UART_IIR_INTID_RDA) {
        if(bufferIndex < MAX_MESSAGE_SIZE) {
            buffer[bufferIndex++] = UART_ReceiveByte(CAN_SERIAL_PORT);
        }
    }
}

}

void readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*)) {
    if(bufferIndex > 0) {
        for(int i = 0; i < bufferIndex && !queue_full(&SERIAL_DEVICE.sendQueue);
                i++) {
            QUEUE_PUSH(uint8_t, &SERIAL_DEVICE.receiveQueue, buffer[i]);
        }
        bufferIndex = 0;
        memset(buffer, 0, MAX_MESSAGE_SIZE);
        processQueue(&SERIAL_DEVICE.receiveQueue, callback);
    }
}

void initializeSerial(SerialDevice* serial) {
	queue_init(&serial->receiveQueue);

	UART_CFG_Type UARTConfigStruct;
	PINSEL_CFG_Type PinCfg;

    // use the 2nd alternative function for UART1 on these pins
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);

	UART_ConfigStructInit(&UARTConfigStruct);
	UARTConfigStruct.Baud_rate = 115200;
	UART_Init(CAN_SERIAL_PORT, &UARTConfigStruct);
	UART_TxCmd(CAN_SERIAL_PORT, ENABLE);

    UART_IntConfig((LPC_UART_TypeDef*)CAN_SERIAL_PORT, UART_INTCFG_RBR, ENABLE);
    NVIC_SetPriority(UART1_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(UART1_IRQn);
}

void processInputQueue(SerialDevice* device) {
    int byteCount = 0;
    uint8_t sendBuffer[queue_length(&device->sendQueue)];
    NVIC_DisableIRQ(UART1_IRQn);
    queue_snapshot(&device->sendQueue, sendBuffer);
    int bytesSent = UART_Send(CAN_SERIAL_PORT, sendBuffer, sizeof(sendBuffer),
            BLOCKING);
    for(int i = 0; i < bytesSent; i++) {
        QUEUE_POP(uint8_t, &device->sendQueue);
    }
    NVIC_EnableIRQ(UART1_IRQn);
}

#endif // __LPC17XX__

#ifdef __LPC17XX__

#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "serialutil.h"
#include "buffers.h"
#include "log.h"

#define CAN_SERIAL_PORT (LPC_UART_TypeDef*)LPC_UART1

void readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*)) {
    // TODO
}

void initializeSerial(SerialDevice* serial) {
	queue_init(&serial->receiveQueue);

	UART_CFG_Type UARTConfigStruct;
	PINSEL_CFG_Type PinCfg;

	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 15;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 16;
	PINSEL_ConfigPin(&PinCfg);

	UART_ConfigStructInit(&UARTConfigStruct);
	UARTConfigStruct.Baud_rate = 115200;
	UART_Init(CAN_SERIAL_PORT, &UARTConfigStruct);
	UART_TxCmd(CAN_SERIAL_PORT, ENABLE);
}

void processInputQueue(SerialDevice* device) {
    int byteCount = 0;
    char sendBuffer[MAX_MESSAGE_SIZE];
    while(!queue_empty(&device->sendQueue) && byteCount < MAX_MESSAGE_SIZE) {
        sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &device->sendQueue);
    }

    // TODO does NON_BLOCKING help here or cause corruption issues?
    UART_Send(CAN_SERIAL_PORT, (uint8_t*)sendBuffer, byteCount, BLOCKING);
}

#endif // __LPC17XX__

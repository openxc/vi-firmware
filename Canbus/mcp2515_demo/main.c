
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <util/delay.h>
#include <stdio.h>
#include <inttypes.h>

#include "uart.h"
#include "mcp2515.h"
#include "global.h"
#include "defaults.h"

// ----------------------------------------------------------------------------

#define	PRINT(string, ...)		printf_P(PSTR(string), ##__VA_ARGS__)

static int putchar__(char c, FILE *stream) {
	uart_putc(c);
	return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(putchar__, 0, _FDEV_SETUP_WRITE);

// ----------------------------------------------------------------------------
void print_can_message(tCAN *message)
{
	uint8_t length = message->header.length;
	
	PRINT("id:     0x%3x\n", message->id);
	PRINT("laenge: %d\n", length);
	PRINT("rtr:    %d\n", message->header.rtr);
	
	if (!message->header.rtr) {
		PRINT("daten:  ");
		
		for (uint8_t i = 0; i < length; i++) {
			PRINT("0x%02x ", message->data[i]);
		}
		PRINT("\n");
	}
}

// ----------------------------------------------------------------------------
// Hauptprogram

int main(void)
{
	// Initialisiere die UART Schnittstelle
	uart_init(UART_BAUD_SELECT(9600UL, F_CPU));
	
	// Aktiviere Interrupts
	sei();
	
	// Umleiten der Standardausgabe => ab jetzt koennen wir printf() verwenden
	stdout = &mystdout;
	
	// Versuche den MCP2515 zu initilaisieren
	if (!mcp2515_init()) {
		PRINT("Fehler: kann den MCP2515 nicht ansprechen!\n");
		for (;;);
	}
	else {
		PRINT("MCP2515 is aktiv\n\n");
	}
	
	PRINT("Erzeuge Nachricht\n");
	tCAN message;
	
	// einige Testwerte
	message.id = 0x123;
	message.header.rtr = 0;
	message.header.length = 2;
	message.data[0] = 0xab;
	message.data[1] = 0xcd;
	
	print_can_message(&message);
	
	PRINT("\nwechsle zum Loopback-Modus\n\n");
	mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), (1<<REQOP1));
	
	// Sende eine Nachricht
	if (mcp2515_send_message(&message)) {
		PRINT("Nachricht wurde in die Puffer geschrieben\n");
	}
	else {
		PRINT("Fehler: konnte die Nachricht nicht senden\n");
	}
	
	// warte ein bisschen
	_delay_ms(10);
	
	if (mcp2515_check_message()) {
		PRINT("Nachricht empfangen!\n");
		
		// read the message from the buffers
		if (mcp2515_get_message(&message)) {
			print_can_message(&message);
			PRINT("\n");
		}
		else {
			PRINT("Fehler: konnte die Nachricht nicht auslesen\n\n");
		}
	}
	else {
		PRINT("Fehler: keine Nachricht empfangen\n\n");
	}
	
	PRINT("zurueck zum normalen Modus\n\n");
	mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
	
	// wir sind ab hier wieder mit dem CAN Bus verbunden
	
	PRINT("Versuche die Nachricht per CAN zu verschicken\n");
	
	// Versuche nochmal die Nachricht zu verschicken, diesmal per CAN
	if (mcp2515_send_message(&message)) {
		PRINT("Nachricht wurde in die Puffer geschrieben\n\n");
	}
	else {
		PRINT("Fehler: konnte die Nachricht nicht senden\n\n");
	}
	
	PRINT("Warte auf den Empfang von Nachrichten\n\n");
	
	while (1) {
		// warten bis wir eine Nachricht empfangen
		if (mcp2515_check_message()) {
			PRINT("Nachricht empfangen!\n");
			
			// Lese die Nachricht aus dem Puffern des MCP2515
			if (mcp2515_get_message(&message)) {
				print_can_message(&message);
				PRINT("\n");
			}
			else {
				PRINT("Kann die Nachricht nicht auslesen\n\n");
			}
		}
	}
	
	return 0;
}

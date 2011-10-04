#include <stdio.h>
#include <NewSoftSerial.h>
#include <mcp2515.h>

#define CANSPEED_125 	7		// CAN speed at 125 kbps
#define CANSPEED_250  	3		// CAN speed at 250 kbps
#define CANSPEED_500	1		// CAN speed at 500 kbps

static FILE uartout = {0};

NewSoftSerial sLCD = NewSoftSerial(3, 6);
#define COMMAND 0xFE
#define CLEAR 0x01
#define LINE0 0x80
#define LINE1 0xC0

int last_fuel_flow = 0;
float last_odo = 0;
float last_mpg = 0;

// Create a output function.
// This works because Serial.write, although of
// type virtual, already exists.
static int
uart_putchar (char c, FILE *stream) {
  Serial.write(c) ;
  return 0 ;
}

void
clear_lcd(void) {
  sLCD.print(COMMAND, BYTE);
  sLCD.print(CLEAR, BYTE);
}

void
print_can_message(tCAN *message) {
  uint8_t length = message->header.length;

  printf("id:     0x%3x\n", message->id);
  printf("length: %d\n", length);
  printf("rtr:    %d\n", message->header.rtr);
	
  if (!message->header.rtr) {
    printf("daten:  ");
		
    for (uint8_t i = 0; i < length; i++) {
      printf("0x%02x ", message->data[i]);
    }
    printf("\n");
  }
}

byte test = 0;

void
send_dummy_message(void) {
  tCAN message;
  
  message.id = 0x200;
  message.header.rtr = 0;
  message.header.length = 8;
  message.data[0] = test;
  message.data[1] = test;
  message.data[2] = test;
  message.data[3] = test;
  message.data[4] = test;
  message.data[5] = test;
  message.data[6] = test;
  message.data[7] = test;
  
  test++;
  
  // Force the system to be in loopback mode!!!!
  mcp2515_bit_modify(CANCTRL, (1 << REQOP2) | (1 << REQOP1) | (1 << REQOP0),
                     (1 << REQOP1));
                     
  if (mcp2515_send_message(&message)) {
  }
}

void
set_filter() {
  // Enter configuration mode.
  mcp2515_bit_modify(CANCTRL, (1 << REQOP2) | (1 << REQOP1) | (1 << REQOP0),
                     (1 << REQOP2) | (0 << REQOP1) | (0 << REQOP0));

  setup_filters();

  // Enter normal mode.
  mcp2515_bit_modify(CANCTRL, (1 << REQOP2) | (1 << REQOP1) | (1 << REQOP0),
                     (0 << REQOP2) | (0 << REQOP1) | (0 << REQOP0));
}

void
setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  
  // XXX parse_database();

  // Fill in the UART file descriptor with pointer to writer.
  fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);

  // The uart is the standard output device STDOUT.
  stdout = &uartout ;
 
  Serial.println("CAN Reader");

  if (mcp2515_init(CANSPEED_500)) {
    Serial.println("CAN Init ok");
  } else {
    Serial.println("Can't init CAN");
  }
  
  set_filter();

  // Put the system into listen only mode.
  mcp2515_bit_modify(CANCTRL, (1 << REQOP2) | (1 << REQOP1) | (1 << REQOP0),
                     (1 << REQOP1) | (1 << REQOP0));

#if 0
  sLCD.begin(9600);
  clear_lcd();
  sLCD.print("Fuel (uL):");
  sLCD.print(COMMAND, BYTE);
  sLCD.print(LINE1, BYTE);
  sLCD.print("Dist (m):");
#endif

  //delay(1000);
}

void
loop() {
  tCAN message;
  uint8_t status;

#if 0
  send_dummy_message();
  delay(1000);
#endif

#if 0
  status = mcp2515_read_register(EFLG);
  if (status & (1 << RX1OVR | 1 << RX0OVR)) {
    Serial.println("Overflow!!!");
    mcp2515_bit_modify(EFLG, (1 << RX1OVR | 1 << RX0OVR), 0);
  }
#endif

  if (mcp2515_check_message()) {
    //Serial.println("Saw message!");
    digitalWrite(13, HIGH);
    if (mcp2515_get_message(&message)) {
      //print_can_message(&message);
      decode_can_message(&message);
    } else {
      printf("Unable to read CAN message!!!!\n");
    }
    digitalWrite(13, LOW);
  }

#if 0
  if (Serial.available()) {
    Serial.print(last_fuel_flow);
    Serial.print(" ");
    Serial.println(last_odo);
    
    Serial.flush();
    
    // Inject a message for testing purposes.
    send_dummy_message();
  }
#endif
}
  

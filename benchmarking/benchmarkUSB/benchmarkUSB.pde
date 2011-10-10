#include <chipKITUSBDevice.h>

// forward reference for the USB constructor
static boolean MY_USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, word size);

// the size of our EP 1 HID buffer
#define GEN_EP              1           // endpoint for data IO
#define USB_EP_SIZE         64          // size or our EP 1 i/o buffers.
#define TOGGLE_LED          0x80        // command from PC to toggle LED state
#define GET_LED_STATE       0x81        // command from PC to get LED state
#define LED                 13          // pin for the LED
#define MIN_BLINK_LOOP_CNT  80000       // the min number of times we must loop before changing the state of the LED while blinking
#define MAX_BLINK_MULT      8           // The max number of times (MAX_BLINK_MULT * MIN_BLINK_LOOP_CNT) through the loop before changing the state of the LED while blinking

USB_HANDLE hHost2Device = 0;		// Handle to the HOST OUT buffer
byte rgHost2Device[USB_EP_SIZE];	// the OUT buffer which is always relative to the HOST, so this is really an in buffer to us

USB_HANDLE hDevice2Host = 0;		// Handle to the HOST OUT buffer
byte rgDevice2Host[USB_EP_SIZE];	// the OUT buffer which is always relative to the HOST, so this is really an in buffer to us

// Create an instance of the USB device
USBDevice usb(MY_USER_USB_CALLBACK_EVENT_HANDLER);	// specify the callback routine
// USBDevice usb(NULL);		// use default callback routine
// USBDevice usb;		// use default callback routine

void setup()
{

	// Enable the serial port for some debugging messages
	Serial.begin(9600);
	Serial.println("call init");

	// This starts the attachement of this USB device to the host.
	// true indicates that we want to wait until we are configured.
	usb.InitializeSystem(true);
	Serial.println("return from init");

	// wait until we get configured
	// this should already be done becasue we said to wait until configured on InitializeSystem
	while(usb.GetDeviceState() < CONFIGURED_STATE);

	Serial.println("Configured, usbOut = ");
	Serial.println((int) hHost2Device, HEX);

	// set the LED as an output parameter
	pinMode(LED, OUTPUT);
}

void loop() {

 	static int ledValue = LOW;

	static int ledBlink = LOW;
	static boolean fBlink = true;
	static int  cBlink = MIN_BLINK_LOOP_CNT;
	static int cBlinkMult = 1;

	// we are armed and waiting for something to come from the Host on EP 1
	// when the handle is no longer busy, that means some data came in.
	if(!usb.HandleBusy(hHost2Device))
	{
	        // debug prints to see what command came in.
		Serial.print("code: ");
		Serial.println(rgHost2Device[0], HEX);

		// It is our sketch convention that the first byte from the PC
		// is an opcode command to tell use what to do.
		switch(rgHost2Device[0])
		{

		        // the toggle LED command
			case TOGGLE_LED:

				// toggle the LED value
				ledValue ^= HIGH;

				// write the new LED value to the LED
				digitalWrite(LED, ledValue);

				// The first time we toggle the LED, we no longer blink.
				fBlink = false;
				break;

			// The Microchip PC application is looking for a push button state, but there is no pushbutton
			// on the chipKIT board, so instead, return the state of the LED
			case GET_LED_STATE:

				rgDevice2Host[0] = GET_LED_STATE;		//Echo back to the host PC the command sent to use so the PC knows we are responding to the get LED request
				rgDevice2Host[1] = ledValue ^ HIGH;		// return the LED state; actually the inverse as this is tricking the default Microchip application as pushbuttons are high when OFF with pullup resistors

				// make sure the HOST has read everything we have sent it, and we can put new stuff in the buffer
				if(!usb.HandleBusy(hDevice2Host))
				{
					hDevice2Host = usb.GenWrite(GEN_EP, rgDevice2Host, USB_EP_SIZE);	// write out our data
				}
				break;

			default:
				break;

		}

                // arm for the next read, it will busy until we get another command on EP 1
                hHost2Device = usb.GenRead(GEN_EP, rgHost2Device, USB_EP_SIZE);
        }

        // blink if we never hit the toggle button on the PC
	if(fBlink && cBlink-- == 0)
	{
		// toggle the blink value
		ledBlink ^= HIGH;
		digitalWrite(LED, ledBlink);  // write out the new value

		// we want an irregular blink so we know the chipKIT is not in the bootloader loop which also blinks the LED.
		// the irregular blink tells us that our code is running and waiting for a command from the PC
		cBlinkMult = cBlinkMult == MAX_BLINK_MULT ? 1 : ++cBlinkMult;
		cBlink = cBlinkMult * MIN_BLINK_LOOP_CNT;
	}
}

/*******************************************************************
 * Function:        BOOL MY_USER_USB_CALLBACK_EVENT_HANDLER(
 *                        USB_EVENT event, void *pdata, WORD size)
 *
 * PreCondition:    None
 *
 * Input:           USB_EVENT event - the type of event
 *                  void *pdata - pointer to the event data
 *                  WORD size - size of the event data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called from the USB stack to
 *                  notify a user application that a USB event
 *                  occured.  This callback is in interrupt context
 *                  when the USB_INTERRUPT option is selected.
 *
 * Note:            None
 *******************************************************************/
static boolean MY_USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, word size) {

    // initial connection up to configure will be handled by the default callback routine.
    usb.DefaultCBEventHandler(event, pdata, size);

    // see what is coming in on the control EP 0
    switch(event)
    {
        case EVENT_TRANSFER:
                //Add application specific callback task or callback function here if desired.
            break;
        case EVENT_SOF:
            break;
        case EVENT_SUSPEND:
            break;
        case EVENT_RESUME:
            break;
        case EVENT_CONFIGURED:

		// Enable Endpoint 1 (HID_EP) for both input and output... 2 endpoints are used for this.
		usb.EnableEndpoint(GEN_EP,USB_OUT_ENABLED|USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

		// set up to wait (arm) for a command to come in on EP 1 (HID_EP)
		rgHost2Device[0] = 0;
		hHost2Device = usb.GenRead(GEN_EP, rgHost2Device, USB_EP_SIZE);

            break;

        case EVENT_SET_DESCRIPTOR:
            break;

        case EVENT_EP0_REQUEST:
            break;

        case EVENT_BUS_ERROR:
            break;
        case EVENT_TRANSFER_TERMINATED:
                //Add application specific callback task or callback function here if desired.
                //The EVENT_TRANSFER_TERMINATED event occurs when the host performs a CLEAR
                //FEATURE (endpoint halt) request on an application endpoint which was
                //previously armed (UOWN was = 1).  Here would be a good place to:
                //1.  Determine which endpoint the transaction that just got terminated was
                //      on, by checking the handle value in the *pdata.
                //2.  Re-arm the endpoint if desired (typically would be the case for OUT
                //      endpoints).
            break;
        default:
            break;
    }
}

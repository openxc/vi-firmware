#include <chipKITUSBDevice.h>

// forward reference for the USB constructor
static boolean MY_USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, word size);

// the size of our EP 1 HID buffer
#define GEN_EP              1           // endpoint for data IO
#define USB_EP_SIZE         64          // size or our EP 1 i/o buffers.
#define SAY_WHAT            0x80        // command from PC to say WHAT

USB_HANDLE hHost2Device = 0;        // Handle to the HOST OUT buffer
byte rgHost2Device[USB_EP_SIZE];    // the OUT buffer which is always relative to the HOST, so this is really an in buffer to us

USB_HANDLE hDevice2Host = 0;        // Handle to the HOST OUT buffer
byte rgDevice2Host[USB_EP_SIZE];    // the OUT buffer which is always relative to the HOST, so this is really an in buffer to us

// Create an instance of the USB device
USBDevice usb(MY_USER_USB_CALLBACK_EVENT_HANDLER);  // specify the callback routine
// USBDevice usb(NULL);     // use default callback routine
// USBDevice usb;       // use default callback routine

void setup() {
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
}

void loop() {
    // we are armed and waiting for something to come from the Host on EP 1
    // when the handle is no longer busy, that means some data came in.
    if(!usb.HandleBusy(hHost2Device)) {
        // debug prints to see what command came in.
        Serial.print("code: ");
        Serial.println(rgHost2Device[0], HEX);

        // It is our sketch convention that the first byte from the PC
        // is an opcode command to tell use what to do.
        switch(rgHost2Device[0]) {
        case SAY_WHAT:
            //Echo back to the host PC the command sent to use so the PC knows
            //we are responding to the same request
            rgDevice2Host[0] = SAY_WHAT;
            rgDevice2Host[1] = 1;

            // make sure the HOST has read everything we have sent it, and we can put new stuff in the buffer
            if(!usb.HandleBusy(hDevice2Host)) {
                hDevice2Host = usb.GenWrite(GEN_EP, rgDevice2Host, USB_EP_SIZE);    // write out our data
            }
            break;

        default:
            break;
        }

        // arm for the next read, it will busy until we get another command on EP 1
        hHost2Device = usb.GenRead(GEN_EP, rgHost2Device, USB_EP_SIZE);
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
            Serial.println("Event: Transfer");
            //Add application specific callback task or callback function here if desired.
            break;

        case EVENT_SOF:
            //Serial.println("Event: Start of Frame");
            break;

        case EVENT_SUSPEND:
            Serial.println("Event: Suspend");
            break;

        case EVENT_RESUME:
            Serial.println("Event: Resume");
            break;

        case EVENT_CONFIGURED:
            Serial.println("Event: Configured");
            // Enable Endpoint 1 (HID_EP) for both input and output... 2 endpoints are used for this.
            usb.EnableEndpoint(GEN_EP,USB_OUT_ENABLED|USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

            // set up to wait (arm) for a command to come in on EP 1 (HID_EP)
            rgHost2Device[0] = 0;
            hHost2Device = usb.GenRead(GEN_EP, rgHost2Device, USB_EP_SIZE);
            break;

        case EVENT_SET_DESCRIPTOR:
            Serial.println("Event: Set Descriptor");
            break;

        case EVENT_EP0_REQUEST:
            Serial.println("Event: EP0 Request");
            break;

        case EVENT_BUS_ERROR:
            Serial.println("Event: Bus Error");
            break;

        case EVENT_TRANSFER_TERMINATED:
            Serial.println("Event: Transfer terminated");
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

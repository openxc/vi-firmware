#include <chipKITUSBDevice.h>

// forward reference for the USB constructor
static boolean usbCallback(USB_EVENT event, void *pdata, word size);
static boolean customUSBCallback(USB_EVENT event);

// the size of our EP 1 HID buffer
#define GEN_EP              1           // endpoint for data IO
#define USB_EP_SIZE         64          // size or our EP 1 i/o buffers.
#define SAY_WHAT            0x80        // command from PC to say WHAT

USB_HANDLE hHost2Device = 0;        // Handle to the HOST OUT buffer
byte rgHost2Device[USB_EP_SIZE];    // the OUT buffer which is always relative to the HOST, so this is really an in buffer to us

USB_HANDLE hDevice2Host = 0;        // Handle to the HOST OUT buffer
byte rgDevice2Host[USB_EP_SIZE];    // the OUT buffer which is always relative to the HOST, so this is really an in buffer to us
USBDevice usb(usbCallback);  // specify the callback routine

extern volatile CTRL_TRF_SETUP SetupPkt;

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

        // arm for the next read, it will busy until we get another command on EP 1
        hHost2Device = usb.GenRead(GEN_EP, rgHost2Device, USB_EP_SIZE);
    }
}

static boolean customUSBCallback(USB_EVENT event, void *pdata, word size) {
    Serial.println("Handling a custom control code");
    switch(SetupPkt.bRequest) {
    case SAY_WHAT:
        Serial.println("WHAT?");
        return true;
    default:
        Serial.print("Didn't recognize event: ");
        Serial.println(SetupPkt.bRequest);
        return false;
    }
}

static boolean usbCallback(USB_EVENT event, void *pdata, word size) {

    // initial connection up to configure will be handled by the default callback routine.
    usb.DefaultCBEventHandler(event, pdata, size);

    // see what is coming in on the control EP 0
    switch(event)
    {
        case EVENT_TRANSFER:
            Serial.println("Event: Transfer completed");
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
            if(!customUSBCallback(event, pdata, size)) {
                Serial.println("Event: Unrecognized EP0 (endpoint 0) Request");
            }
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

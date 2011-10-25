#include <chipKITUSBDevice.h>

// forward reference for the USB constructor
static boolean usbCallback(USB_EVENT event, void *pdata, word size);
static boolean customUSBCallback(USB_EVENT event);

#define DATA_ENDPOINT 1
#define DATA_ENDPOINT_BUFFER_SIZE 65

USB_HANDLE handleInput = 0;
byte message[] = "{\"name\":\"SteeringWheelAngle\",\"value\":45}";
int messageSize;
uint8_t messageBuffer[DATA_ENDPOINT_BUFFER_SIZE];

USBDevice usb(usbCallback);

// This is a reference tot he last packet read, I believe
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

    Serial.println("Configured, usbIn = ");
    Serial.println((int) handleInput, HEX);

    messageSize = strlen((char*)message);
    Serial.print("Message is ");
    Serial.print(messageSize, DEC);
    Serial.println(" bytes");

    for(int i = 0; i < messageSize; i++)  {
        messageBuffer[i] = message[i];
    }
}

void loop() {
    // using our own loop seems faster than using the arduino's
    while(true) {
        // when the handle is no longer busy, that means the host read some
        // data and we can write more. In USB parlance, "input" goes from the
        // device to the host.
        while(usb.HandleBusy(handleInput));

        handleInput = usb.GenWrite(DATA_ENDPOINT, messageBuffer, messageSize);
    }
}

static boolean customUSBCallback(USB_EVENT event, void* pdata, word size) {
    Serial.println("Handling a custom control code");
    int newMessageSize = messageSize;
    switch(SetupPkt.bRequest) {
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
            //Serial.println("Event: Transfer completed");
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
            // Enable DATA_ENDPOINT for input
            usb.EnableEndpoint(DATA_ENDPOINT,
                    USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
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

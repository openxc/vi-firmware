#include "telit_he910.h"
#include "telit_he910_platforms.h"
#include "interface/uart.h"
#include "WProgram.h"
#include "util/log.h"
#include "util/timer.h"
#include "gpio.h"
#include "config.h"
#include "can/canread.h"
#include "commands/commands.h"
#include "interface/interface.h"
#include "http.h"
#include <string.h>
#include <stdio.h>

namespace gpio = openxc::gpio;
namespace uart = openxc::interface::uart;
namespace can = openxc::can; // using this to get publishVehicleMessage() for GPS...no sense re-inventing the wheel
namespace http = openxc::http;
namespace telit = openxc::telitHE910;
namespace commands = openxc::commands;

using openxc::interface::uart::UartDevice;
using openxc::gpio::GpioValue;
using openxc::gpio::GPIO_DIRECTION_OUTPUT;
using openxc::gpio::GPIO_DIRECTION_INPUT;
using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_VALUE_LOW;
using openxc::util::time::delayMs;
using openxc::util::log::debug;
using openxc::util::time::uptimeMs;
using openxc::config::getConfiguration;
using openxc::telitHE910::TELIT_CONNECTION_STATE;
using openxc::payload::PayloadFormat;

/*PRIVATE MACROS*/

#define TELIT_MAX_MESSAGE_SIZE         512
#define NETWORK_CONNECT_TIMEOUT     150000
#define PDP_MAX_ATTEMPTS                 3

/*PRIVATE VARIABLES*/

static unsigned int bauds[3] = {
    230400,
    115200,
    57600
};

static const char* gps_fix_enum[openxc::telitHE910::FIX_MAX_ENUM] = {"NO_FIX_0", "NO_FIX", "2D_FIX", "3D_FIX"};

static char recv_data[1024];    // common buffer for receiving data from the modem
static char* pRx = recv_data;
static TelitDevice* telitDevice;
static bool connect = false;
static uint8_t sendBuffer[SEND_BUFFER_SIZE];
static uint8_t* pSendBuffer = sendBuffer;

static TELIT_CONNECTION_STATE state = telit::POWER_OFF;

/*PRIVATE FUNCTIONS*/

static bool autobaud(openxc::telitHE910::TelitDevice* device);
static void telit_setIoDirection(void);
static void setPowerState(bool enable);
static bool sendCommand(TelitDevice* device, const char* command, const char* response, uint32_t timeoutMs);
static bool sendCommand(TelitDevice* device, const char* command, const char* response, const char* error, uint32_t timeoutMs);
static void sendData(TelitDevice* device, char* data, unsigned int len);
static void clearRxBuffer(void);
static bool getResponse(const char* startToken, const char* stopToken, char* response, unsigned int maxLen);
static bool parseGPSACP(const char* GPSACP);

namespace openxc {
namespace telitHE910 {

static TELIT_CONNECTION_STATE DSM_Power_Off(TelitDevice* device);
static TELIT_CONNECTION_STATE DSM_Power_On_Delay(TelitDevice* device);
static TELIT_CONNECTION_STATE DSM_Power_On(TelitDevice* device);
static TELIT_CONNECTION_STATE DSM_Power_Up_Delay(TelitDevice* device);
static TELIT_CONNECTION_STATE DSM_Initialize(TelitDevice* device);
static TELIT_CONNECTION_STATE DSM_Wait_For_Network(TelitDevice* device);
static TELIT_CONNECTION_STATE DSM_Close_PDP(TelitDevice* device);
static TELIT_CONNECTION_STATE DSM_Open_PDP_Delay(TelitDevice* device);
static TELIT_CONNECTION_STATE DSM_Open_PDP(TelitDevice* device);
static TELIT_CONNECTION_STATE DSM_Ready(TelitDevice* device);

}
}

/*DEVICE STATE MACHINE*/

TELIT_CONNECTION_STATE openxc::telitHE910::getDeviceState() {
    return state;
}

TELIT_CONNECTION_STATE openxc::telitHE910::connectionManager(TelitDevice* device) 
{    
    switch(state)
    {
        case POWER_OFF:
            state = DSM_Power_Off(device);
            break;
            
        case POWER_ON_DELAY:
            state = DSM_Power_On_Delay(device);
            break;
            
        case POWER_ON:
            state = DSM_Power_On(device);
            break;
            
        case POWER_UP_DELAY:
            state = DSM_Power_Up_Delay(device);
            break;
            
        case INITIALIZE:
            state = DSM_Initialize(device);
            break;
            
        case WAIT_FOR_NETWORK:
            state = DSM_Wait_For_Network(device);
            break;
            
        case CLOSE_PDP:
            state = DSM_Close_PDP(device);
            break;
            
        case OPEN_PDP_DELAY:
            state = DSM_Open_PDP_Delay(device);
            break;
            
        case OPEN_PDP:
            state = DSM_Open_PDP(device);
            break;
            
        case READY:
            state = DSM_Ready(device);
            break;
            
        default:
            state = POWER_OFF;
            break;
    }
    
    return state;
}

void openxc::telitHE910::deinitialize() {
    state = POWER_OFF;
    setPowerState(false);
}

bool openxc::telitHE910::connected(TelitDevice* device) {
    return connect;
}

static bool autobaud(openxc::telitHE910::TelitDevice* device) {

    bool rc = false;
    unsigned int i = 0;

    // loop through a set of baud rates looking for a valid response
    for(i = 0; i < 3; ++i)
    {
        // set local baud rate to next attempt
        uart::changeBaudRate(device->uart, bauds[i]);
        
        // attempt set the remote baud rate to desired (config.h) value
        if(openxc::telitHE910::setBaud(UART_BAUD_RATE) == true)
        {
            // match local baud to desired value
            uart::changeBaudRate(device->uart, UART_BAUD_RATE);
            
            // we're good!
            rc = true;
            break;
        }
    }
    
    return rc;

}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Power_Off(TelitDevice* device) {

    TELIT_CONNECTION_STATE l_state = POWER_OFF;

    device->descriptor.type = openxc::interface::InterfaceType::TELIT;
        
    setPowerState(false);
    telitDevice = device;
    l_state = POWER_ON_DELAY;
    
    return l_state;

}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Power_On_Delay(TelitDevice* device) {

    TELIT_CONNECTION_STATE l_state = POWER_ON_DELAY;
    static unsigned int sub_state = 0;
    static unsigned int timer = 0;
    
    switch(sub_state)
    {
        case 0:
        
            timer = uptimeMs() + 1000;
            sub_state = 1;
            
            break;
            
        case 1:
        
            if(uptimeMs() > timer)
            {
                l_state = POWER_ON;
                sub_state = 0;
            }
            
            break;

//         default: //Do nothing by default
//             break;
    }
    
    return l_state;

}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Power_On(TelitDevice* device) {

    TELIT_CONNECTION_STATE l_state = POWER_ON;
    
    setPowerState(true);
    l_state = POWER_UP_DELAY;
    
    return l_state;

}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Power_Up_Delay(TelitDevice* device) {

    TELIT_CONNECTION_STATE l_state = POWER_UP_DELAY;
    static unsigned int sub_state = 0;
    static unsigned int timer = 0;

    switch(sub_state)
    {
        case 0:
        
            timer = uptimeMs() + 8500;
            sub_state = 1;
            
            break;
            
        case 1:
        
            if(uptimeMs() > timer)
            {
                l_state = INITIALIZE;
                sub_state = 0;
            }
            
            break;

//         default: //Do nothing by default
//             break;
    }
    
    return l_state;

}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Initialize(TelitDevice* device) {

    TELIT_CONNECTION_STATE l_state = INITIALIZE;
    static unsigned int sub_state = 0;
    static unsigned int timer = 0;
    static unsigned int SIMstatus = 0;
    static char ICCID[32];
    static char IMEI[32];
    
    switch(sub_state)
    {
        case 0:
        
            // figure out the baud rate
            if(autobaud(device))
            {
                timer = uptimeMs() + 1000;
                sub_state = 1;
            }
            else
            {
                debug("Failed to set the baud rate for Telit HE910...is the device connected to 12V power?");
                l_state = POWER_OFF;
                break;
            }
        
            break;
            
        case 1:
        
            if(uptimeMs() > timer)
            {
                sub_state = 2;
            }
        
            break;
            
        case 2:
            
            // save settings
            if(saveSettings() == false)
            {
                debug("Failed to save modem settings, continuing with device initialization.");
            }
            
            // check SIM status
            if(getSIMStatus(&SIMstatus) == false)
            {
                sub_state = 0;
                l_state = POWER_OFF;
                break;
            }
            
            // start the GPS chip
            if(device->config.globalPositioningSettings.gpsEnable)
            {
                if(setGPSPowerState(true) == false)
                {
                    sub_state = 0;
                    l_state = POWER_OFF;
                    break;
                }
            }
            
            // make sure SIM is installed, else exit
            if(SIMstatus != 1)
            {
                debug("SIM not detected, aborting device initialization.");
                sub_state = 0;
                l_state = POWER_OFF;
                break;
            }
            
            // get device identifier (IMEI)
            if(getDeviceIMEI(IMEI) == false)
            {
                sub_state = 0;
                l_state = POWER_OFF;
                break;
            }
            memcpy(device->deviceId, IMEI, strlen(IMEI) < MAX_DEVICE_ID_LENGTH ? strlen(IMEI) : MAX_DEVICE_ID_LENGTH);
            
            // get SIM number (ICCID)
            if(getICCID(ICCID) == false)
            {
                sub_state = 0;
                l_state = POWER_OFF;
                break;
            }
            memcpy(device->ICCID, ICCID, strlen(ICCID) < MAX_ICCID_LENGTH ? strlen(ICCID) : MAX_ICCID_LENGTH);
            
            // set mobile operator connect mode
            if(setNetworkConnectionMode(device->config.networkOperatorSettings.operatorSelectMode, device->config.networkOperatorSettings.networkDescriptor) == false)
            {
                sub_state = 0;
                l_state = POWER_OFF;
                break;
            }
            
            // configure data session
            if(configurePDPContext(device->config.networkDataSettings) == false)
            {
                sub_state = 0;
                l_state = POWER_OFF;
                break;
            }
            
            // configure a single TCP/IP socket
            if(configureSocket(1, device->config.socketConnectSettings) == false)
            {
                sub_state = 0;
                l_state = POWER_OFF;
                break;
            }
            
            sub_state = 0;
            l_state = WAIT_FOR_NETWORK;
            
            break;

        default: //Do nothing by default
            break;
    }
    
    return l_state;

}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Wait_For_Network(TelitDevice* device) {
    
    TELIT_CONNECTION_STATE l_state = WAIT_FOR_NETWORK;
    static unsigned int sub_state = 0;
    static unsigned int timer = 0;
    static unsigned int timeout = 0;
    static NetworkDescriptor current_network = {};
    static NetworkConnectionStatus connectStatus = UNKNOWN;
    
    switch(sub_state)
    {
        case 0:
    
            timeout = uptimeMs() + NETWORK_CONNECT_TIMEOUT;
            sub_state = 1;
    
            break;
    
        case 1:
            
            if(getNetworkConnectionStatus(&connectStatus))
            {
                if(connectStatus == REGISTERED_HOME || (device->config.networkOperatorSettings.allowDataRoaming && connectStatus == REGISTERED_ROAMING))
                {
                    if(getCurrentNetwork(&current_network))
                    {
                        debug("Telit connected to PLMN %u, access type %u", current_network.PLMN, current_network.networkType);
                    }
                    sub_state = 0;
                    l_state = CLOSE_PDP;
                }
                else
                {
                    timer = uptimeMs() + 500;
                    sub_state = 2;
                }
            }
            
            break;
            
        case 2:
        
            if(uptimeMs() > timeout)
            {
                sub_state = 0;
                l_state = POWER_OFF;
            }
            else if(uptimeMs() > timer)
            {
                sub_state = 1;
            }
        
            break;

//         default: //Do nothing by default
//             break;
    }
    
    return l_state;
    
}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Close_PDP(TelitDevice* device) {
    
    TELIT_CONNECTION_STATE l_state = CLOSE_PDP;
    static unsigned int sub_state = 0;
    
    // deactivate data session (just in case the network thinks we still have an active PDP context)
    if(closePDPContext())
    {
        sub_state = 0;
        l_state = OPEN_PDP_DELAY;
    }
    else
    {    
        sub_state = 0;
        l_state = POWER_OFF;
    }
    
    return l_state;
    
}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Open_PDP_Delay(TelitDevice* device) {
    
    TELIT_CONNECTION_STATE l_state = OPEN_PDP_DELAY;
    static unsigned int sub_state = 0;
    static unsigned int timer = 0;
    
    switch(sub_state)
    {
        case 0:
        
            timer = uptimeMs() + 1000;
            sub_state = 1;
            
            break;
            
        case 1:

            if(uptimeMs() > timer)
            {
                l_state = OPEN_PDP;
                sub_state = 0;
            }
            
            break;

//         default: //Do nothing by default
//             break;
    }
    
    return l_state;
    
}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Open_PDP(TelitDevice* device) {
    
    TELIT_CONNECTION_STATE l_state = OPEN_PDP;
    static uint8_t pdp_counter = 0;
    
    // activate data session
    if(openPDPContext())
    {
        pdp_counter = 0;
        l_state = READY;
    }
    else
    {    
        if(pdp_counter < PDP_MAX_ATTEMPTS)
        {
            pdp_counter++;
            l_state = CLOSE_PDP;
        }
        else
        {
            pdp_counter = 0;
            l_state = POWER_OFF;
        }
    }
    
    return l_state;
}

static TELIT_CONNECTION_STATE openxc::telitHE910::DSM_Ready(TelitDevice* device) 
{
    TELIT_CONNECTION_STATE l_state = READY;
    static unsigned int sub_state = 0;
    static unsigned int timer = 0;
    bool pdp_connected = false;
    NetworkConnectionStatus connectStatus = UNKNOWN;

    switch(sub_state)
    {
        case 0:
        
            if(getNetworkConnectionStatus(&connectStatus), (connectStatus != REGISTERED_HOME && connectStatus != REGISTERED_ROAMING))
            {
                debug("Modem has lost network connection");
                connect = false;
                l_state = WAIT_FOR_NETWORK;
            }
            else if(getPDPContext(&pdp_connected), !pdp_connected)
            {
                debug("Modem has lost data session");
                connect = false;
                l_state = CLOSE_PDP;
            }
            else
            {
                connect = true;
                timer = uptimeMs() + 1000;
                sub_state = 1;
            }
        
            break;
            
        case 1:
        
            if(uptimeMs() > timer)
                sub_state = 0;
        
            break;

//         default: //Do nothing by default
//             break;
    }
    
    return l_state;
}

/*MODEM AT COMMANDS*/

bool openxc::telitHE910::saveSettings() {

    bool rc = true;
    
    if(sendCommand(telitDevice, "AT&W0\r\n", "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::setBaud(unsigned int baudRate) {

    bool rc = true;
    char command[32] = {};
    
    sprintf(command, "AT+IPR=%u\r\n", baudRate);
    if(sendCommand(telitDevice, command, "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::getDeviceFirmwareVersion(char* firmwareVersion) {

    bool rc = true;
    
    if(sendCommand(telitDevice, "AT+CGMR\r\n", "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("AT+CGMR\r\n\r\n", "\r\n\r\nOK\r\n", firmwareVersion, 31) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::getSIMStatus(unsigned int* status) {

    bool rc = true;
    char temp[8];

    if(sendCommand(telitDevice, "AT#QSS?\r\n", "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("#QSS: ", "\r\n\r\n", temp, 7) == false) {
        rc = false;
        return rc;
    }
    *status = atoi(&temp[2]);


}

bool openxc::telitHE910::getICCID(char* ICCID) {

    bool rc = true;

    if(sendCommand(telitDevice, "AT#CCID\r\n", "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("#CCID: ", "\r\n\r\n", ICCID, 31) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::getDeviceIMEI(char* IMEI) {

    bool rc = true;

    if(sendCommand(telitDevice, "AT+CGSN\r\n", "\r\n\r\nOK\r\n", 2000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("AT+CGSN\r\n\r\n", "\r\n\r\nOK\r\n", IMEI, 31) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::setNetworkConnectionMode(OperatorSelectMode mode, NetworkDescriptor network) {

    bool rc = true;
    char command[64] = {};

    switch(mode) {
        case AUTOMATIC:
        
            sprintf(command, "AT+COPS=0,2\r\n");

            break;
            
        case MANUAL:
        
            sprintf(command, "AT+COPS=1,2,%u,%u\r\n", network.PLMN, network.networkType);
        
            break;
            
        case DEREGISTER:
        
            sprintf(command, "AT+COPS=2,2");
            
            break;
            
        case SET_ONLY:
        
            sprintf(command, "AT+COPS=3,2");
            
            break;
            
        case MANUAL_AUTOMATIC:
        
            sprintf(command, "AT+COPS=4,2,%u,%u\r\n", network.PLMN, network.networkType);
            
            break;
            
        default:
        
            debug("Modem received invalid operator select mode");
            
            break;
    }
    
    if(sendCommand(telitDevice, command, "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::getNetworkConnectionStatus(NetworkConnectionStatus* status) {

    bool rc = true;
    char temp[8] = {};
    
    if(sendCommand(telitDevice, "AT+CREG?\r\n", "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("+CREG: ", "\r\n\r\nOK\r\n", temp, 7) == false) {
        rc = false;
        return rc;
    }
    *status = (NetworkConnectionStatus)atoi(&temp[2]);

}

bool openxc::telitHE910::getCurrentNetwork(NetworkDescriptor* network) {

    bool rc = true;
    char temp[32] = {};
    char* p = NULL;
    
    // response: +COPS: <mode>,<format>,<oper>,<AcT>
    
    if(sendCommand(telitDevice, "AT+COPS?\r\n", "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("+COPS: ", "\r\n\r\nOK\r\n", temp, 31) == false) {
        rc = false;
        return rc;
    }
    if(p = strchr(temp, ','), p) {
        p++;
        if(p = strchr(p, ','), p) {
            p+=2;
            network->PLMN = atoi(p);
            if(p = strchr(p, ','), p) {
                p++;
                network->networkType = (openxc::telitHE910::NetworkType)atoi(p);
            }
        }
    }

}

bool openxc::telitHE910::configurePDPContext(NetworkDataSettings dataSettings) {

    bool rc = true;
    char command[64] = {};
    
    sprintf(command,"AT+CGDCONT=1,\"IP\",\"%s\"\r\n", dataSettings.APN);
    if(sendCommand(telitDevice, command, "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::configureSocket(unsigned int socketNumber, SocketConnectSettings socketSettings) {

    bool rc = true;
    char command[64] = {};
    
    if(socketNumber == 0 || socketNumber > 6) {
        rc = false;
        return rc;
    }
    
    sprintf(command,"AT#SCFG=%u,1,%u,%u,%u,%u\r\n", socketNumber, socketSettings.packetSize, 
        socketSettings.idleTimeout, socketSettings.connectTimeout, socketSettings.txFlushTimer);
    if(sendCommand(telitDevice, command, "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::openPDPContext() {

    bool rc = true;
    
    if(sendCommand(telitDevice, "AT#SGACT=1,1\r\n", "\r\n\r\nOK\r\n", "ERROR", 30000) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::closePDPContext() {

    bool rc = true;
    
    if(sendCommand(telitDevice, "AT#SGACT=1,0\r\n", "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::getPDPContext(bool* connected) {

    bool rc = true;
    char temp[32] = {};
    
    if(sendCommand(telitDevice, "AT#SGACT?\r\n", "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("#SGACT: 1,", "\r\n\r\nOK\r\n", temp, 31) == false) {
        rc = false;
        return rc;
    }
    *connected = (bool)atoi(temp);

}

bool openxc::telitHE910::openSocket(unsigned int socketNumber, ServerConnectSettings serverSettings) {

    bool rc = true;
    char command[128] = {};

    sprintf(command,"AT#SD=%u,0,%u,\"%s\",255,1,1\r\n", socketNumber, serverSettings.port, serverSettings.host);
    if(sendCommand(telitDevice, command, "\r\n\r\nOK\r\n", "ERROR", 15000) == false) {
        rc = false;
        return rc;
    }
    
}

bool openxc::telitHE910::isSocketOpen(unsigned int socketNumber) {

    SocketStatus status;
    
    if(getSocketStatus(socketNumber, &status)) {
        return (status != SOCKET_CLOSED);
    } else {
        return false;
    }

}

bool openxc::telitHE910::closeSocket(unsigned int socketNumber) {

    bool rc = true;
    char command[16] = {};

    sprintf(command,"AT#SH=%u\r\n", socketNumber);
    if(sendCommand(telitDevice, command, "\r\n\r\nOK\r\n", "ERROR", 5000) == false) {
        rc = false;
        return rc;
    }
    
}

bool openxc::telitHE910::getSocketStatus(unsigned int socketNumber, SocketStatus* status) {

    bool rc = true;
    char command[16] = {};
    char temp[8] = {};
    
    sprintf(command, "AT#SS=%u\r\n", socketNumber);
    if(sendCommand(telitDevice, command, "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("#SS: ", "\r\n\r\nOK\r\n", temp, 7) == false) {
        rc = false;
        return rc;
    }
    *status = (SocketStatus)atoi(&temp[2]);

}

bool openxc::telitHE910::isSocketDataAvailable(unsigned int socketNumber) {

    SocketStatus status;
    
    if(getSocketStatus(socketNumber, &status)) {
        return status == SOCKET_SUSPENDED_DATA_PENDING;
    } else {
        return false;
    }

}

bool openxc::telitHE910::writeSocket(unsigned int socketNumber, char* data, unsigned int* len) {

    bool rc = true;
    char command[32];
    unsigned long timer = 0;
    unsigned int i = 0;
    int rx_byte = 0;
    unsigned int maxWrite = 0;
    
    // calculate max bytes to write
    maxWrite = (*len > TELIT_MAX_MESSAGE_SIZE) ? TELIT_MAX_MESSAGE_SIZE : *len;
    
    // issue the socket write command
    sprintf(command, "AT#SSENDEXT=%u,%u\r\n", socketNumber, maxWrite);
    if(sendCommand(telitDevice, command, "> ", 5000) == false) {
        rc = false;
        return rc;
    }
    
    // clear the receive buffer
    clearRxBuffer();
    
    // send data through the socket
    sendData(telitDevice, data, maxWrite);
    
    // start the receive timer
    timer = uptimeMs() + 10000;
    
    // read out the socket data echo (don't need to store it)
    while(1) {
        if(rx_byte = uart::readByte(telitDevice->uart), rx_byte > -1) {
            ++i;
        }
        if(i == maxWrite) {
            break;
        }
        if(uptimeMs() >= timer) {
            rc = false;
            return rc;
        }
    }

    // get the OK
    while(1) {
        if(rx_byte = uart::readByte(telitDevice->uart), rx_byte > -1) {
            *pRx++ = rx_byte;
        }
        if(strstr(recv_data, "OK")) {
            rc = true;
            break;
        }
        if(uptimeMs() >= timer) {
            rc = false;
            return rc;
        }
    }

    // update write count
    *len = maxWrite;

}

bool openxc::telitHE910::readSocket(unsigned int socketNumber, char* data, unsigned int* len) {

    bool rc = true;
    char command[32] = {};
    char reply[32] = {};
    char* pS = NULL;
    unsigned int readCount = 0;
    unsigned int i = 0;
    unsigned int maxRead = 0;
    int rx_byte = -1;
    unsigned long timer = 0;
    
    // calculate max bytes to read
    maxRead = (*len > TELIT_MAX_MESSAGE_SIZE) ? TELIT_MAX_MESSAGE_SIZE : *len;
    
    // issue the socket read command
    sprintf(command, "AT#SRECV=%u,%u\r\n", socketNumber, maxRead);
    sprintf(reply, "#SRECV: %u,", socketNumber);
    if(sendCommand(telitDevice, command, reply, 1000) == false) {
        rc = false;
        return rc;
    }
    
    // start the receive timer
    timer = uptimeMs() + 10000;
    
    // find the start of the reply
    pS = recv_data;
    if(pS = strstr(pS, reply), !pS) {
        rc = false;
        return rc;
    }
    pS += 10;
    pRx = pS;
    
    // read to end of line
    while(1) {
        if(rx_byte = uart::readByte(telitDevice->uart), rx_byte > -1) {
            *pRx++ = rx_byte;
        }
        if(strstr(pS, "\r\n")) {
            break;
        }
        if(uptimeMs() >= timer) {
            rc = false;
            return rc;
        }
    }    
    // get the read count
    readCount = atoi(pS);
    pS = pRx;
    
    // read the socket data
    i = 0;
    while(1) {
        if(rx_byte = uart::readByte(telitDevice->uart), rx_byte > -1) {
            *pRx++ = rx_byte;
            ++i;
        }
        if(i == readCount) {
            break;
        }
        if(uptimeMs() >= timer) {
            rc = false;
            return rc;
        }
    }
    // put the read data into caller
    memcpy(data, pS, readCount);
    *len = readCount;
    pS = pRx;
    
    // finish with OK
    while(1) {
        if(rx_byte = uart::readByte(telitDevice->uart), rx_byte > -1) {
            *pRx++ = rx_byte;
        }
        if(strstr(pS, "\r\n\r\nOK\r\n")) {
            break;
        }
        if(uptimeMs() >= timer) {
            rc = false;
            return rc;
        }
    }

}

bool openxc::telitHE910::readSocketOne(unsigned int socketNumber, char* data, unsigned int* len) {

    bool rc = true;
    char command[32] = {};
    char reply[32] = {};
    char* pS = NULL;
    unsigned int readCount = 0;
    unsigned int i = 0;
    unsigned int maxRead = 0;
    int rx_byte = -1;
    unsigned long timer = 0;
    
    // calculate max bytes to read
    maxRead = (*len > 1) ? 1 : *len;
    
    // issue the socket read command
    sprintf(command, "AT#SRECV=%u,%u\r\n", socketNumber, maxRead);
    sprintf(reply, "#SRECV: %u,", socketNumber);
    if(sendCommand(telitDevice, command, reply, 1000) == false) {
        rc = false;
        return rc;
    }
    
    // start the receive timer
    timer = uptimeMs() + 10000;
    
    // find the start of the reply
    pS = recv_data;
    if(pS = strstr(pS, reply), !pS) {
        rc = false;
        return rc;
    }
    pS += 10;
    pRx = pS;
    
    // read to end of line
    while(1) {
        if(rx_byte = uart::readByte(telitDevice->uart), rx_byte > -1) {
            *pRx++ = rx_byte;
        }
        if(strstr(pS, "\r\n")) {
            break;
        }
        if(uptimeMs() >= timer) {
            rc = false;
            return rc;
        }
    }    
    // get the read count
    readCount = atoi(pS);
    pS = pRx;
    
    // read the socket data
    i = 0;
    while(1) {
        if(rx_byte = uart::readByte(telitDevice->uart), rx_byte > -1) {
            *pRx++ = rx_byte;
            ++i;
        }
        if(i == readCount) {
            break;
        }
        if(uptimeMs() >= timer) {
            rc = false;
            return rc;
        }
    }
    // put the read data into caller
    memcpy(data, pS, readCount);
    *len = readCount;
    pS = pRx;
    
    // finish with OK
    while(1) {
        if(rx_byte = uart::readByte(telitDevice->uart), rx_byte > -1) {
            *pRx++ = rx_byte;
        }
        if(strstr(pS, "\r\n\r\nOK\r\n")) {
            break;
        }
        if(uptimeMs() >= timer) {
            rc = false;
            return rc;
        }
    }

}

/*SEND/RECEIVE*/

static bool sendCommand(TelitDevice* device, const char* command, const char* response, uint32_t timeoutMs) {

    bool rc = false;
    
    unsigned int tx_cnt = 0;
    unsigned int tx_size = 0;
    
    int rx_byte = 0;
    
    unsigned long timer = 0;
    
    // clear the receive buffer
    clearRxBuffer();
    
    // send the command
    tx_size = strlen(command);
    for(tx_cnt = 0; tx_cnt < tx_size; ++tx_cnt) {
        uart::writeByte(device->uart, command[tx_cnt]);
    }
    
    // start the receive timer
    timer = uptimeMs();
    
    // receive the response
    while(uptimeMs() - timer < timeoutMs) {
        if(rx_byte = uart::readByte(device->uart), rx_byte > -1) {
            *pRx++ = rx_byte;
        }
        if(strstr(recv_data, response)) {
            rc = true;
            break;
        }
    }
    
    return rc;

}

static bool sendCommand(TelitDevice* device, const char* command, const char* response, const char* error, uint32_t timeoutMs) {

    bool rc = false;
    
    unsigned int tx_cnt = 0;
    unsigned int tx_size = 0;
    
    int rx_byte = 0;
    
    unsigned long timer = 0;
    
    // clear the receive buffer
    clearRxBuffer();
    
    // send the command
    tx_size = strlen(command);
    for(tx_cnt = 0; tx_cnt < tx_size; ++tx_cnt) {
        uart::writeByte(device->uart, command[tx_cnt]);
    }
    
    // start the receive timer
    timer = uptimeMs();
    
    // receive the response
    while(uptimeMs() - timer < timeoutMs) {
        if(rx_byte = uart::readByte(device->uart), rx_byte > -1) {
            *pRx++ = rx_byte;
        }
        if(strstr(recv_data, response)) {
            rc = true;
            break;
        }
        else if(strstr(recv_data, error)) {
            rc = false;
            break;
        }
    }
    
    return rc;

}

static void clearRxBuffer() {

    // purge the HardwareSerial buffer
    ((HardwareSerial*)telitDevice->uart->controller)->purge();

    // clear the modem buffer
    memset(recv_data, 0x00, 256);
    pRx = recv_data;
    
    return;

}

static bool getResponse(const char* startToken, const char* stopToken, char* response, unsigned int maxLen) {
    
    bool rc = true;
    
    char* p1 = NULL;
    char* p2 = NULL;
    
    if(p1 = strstr(recv_data, startToken), !p1) {
        rc = false;
        return rc;
    }
    if(p2 = strstr(recv_data, stopToken), !p2) {
        rc = false;
        return rc;
    }
    p1 += strlen(startToken);
    memcpy(response, p1, (maxLen < p2-p1) ? (maxLen) : (p2-p1));

}

static void sendData(TelitDevice* device, char* data, unsigned int len) {

    unsigned int i = 0;
    
    for(i = 0; i < len; ++i) {
        uart::writeByte(device->uart, data[i]);
        // as I send bytes I'll bet there are bytes coming back and I don't know how the hardware interface is
        // handling them. I assume there is more than the HW registers available under the hood, but if that buffer fills // before I return.....well I'm screwed as bytes will drop
        // the HardwareSerial implementation (cores/pic32) has a 512 byte ring buffer
    }
    
    return;

}

/*MODEM POWER MANAGEMENT*/

// Only want to set the directly once because it flips the power on/off.
#ifdef TELIT_HE910_ENABLE_SUPPORT
static void telit_setIoDirection() {
    static bool directionSet = false;
    if(!directionSet) {
        // be aware that setting the direction here will default it to the off
        // state, so the Bluetooth module will go *off* and then back *on*
        gpio::setDirection(TELIT_HE910_ENABLE_PORT, TELIT_HE910_ENABLE_PIN,
                GPIO_DIRECTION_OUTPUT);
        directionSet = true;
    }
}
#endif

static void setPowerState(bool enabled) {

    #ifdef TELIT_HE910_ENABLE_SUPPORT
    
    enabled = TELIT_HE910_ENABLE_PIN_POLARITY ? enabled : !enabled;
    debug("Turning Telit HE910 %s", enabled ? "on" : "off");
    telit_setIoDirection();
    gpio::setValue(TELIT_HE910_ENABLE_PORT, TELIT_HE910_ENABLE_PIN,
            enabled ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
    
    #endif
    
    return;

}

/*GPS*/

bool openxc::telitHE910::setGPSPowerState(bool enable) {

    bool rc = true;
    char command[64] = {};
    
    sprintf(command, "AT$GPSP=%u\r\n", (unsigned int)enable);
    if(sendCommand(telitDevice, command, "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }

}

bool openxc::telitHE910::getGPSPowerState(bool* enable) {

    bool rc = true;
    char command[64] = {};
    char temp[8];
    
    sprintf(command, "AT$GPSP?\r\n");
    if(sendCommand(telitDevice, command, "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("$GPSP: ", "\r\n\r\nOK\r\n", temp, 7) == false) {
        rc = false;
        return rc;
    }
    *enable = (bool)atoi(&temp[0]);

}

bool openxc::telitHE910::getGPSLocation() {

    bool rc = true;
    char temp[128] = {};
    static unsigned long next_update = 0;
    
    if(uptimeMs() < next_update) {
        return rc;
    }

    // retrieve the GPS location string from the modem
    if(sendCommand(telitDevice, "AT$GPSACP\r\n", "\r\n\r\nOK\r\n", 1000) == false) {
        rc = false;
        return rc;
    }
    if(getResponse("$GPSACP: ", "\r\n\r\nOK\r\n", temp, 127) == false) {
        rc = false;
        return rc;
    }
    
    // now we have the GPS string in 'temp', send to parser to publish signals
    rc = parseGPSACP(temp);
    
    next_update = uptimeMs() + getConfiguration()->telit->config.globalPositioningSettings.gpsInterval;

}

static void publishGPSSignal(const char* field_name, char* field_value, openxc::pipeline::Pipeline* pipeline) {
    can::read::publishStringMessage(field_name, field_value, pipeline);
}

static void publishGPSSignal(const char* field_name, float field_value, openxc::pipeline::Pipeline* pipeline) {
    can::read::publishNumericalMessage(field_name, field_value, pipeline);
}

static bool parseGPSACP(const char* GPSACP) {

    bool rc = true;
    char* p1 = NULL;
    char* p2 = NULL;
    
    // pass these to publishVehicleMessage
    char tmp[8] = {};
    float field_value_numerical = 0;
    openxc::pipeline::Pipeline* pipeline;
    openxc::telitHE910::GlobalPositioningSettings* gpsConfig;
    
    pipeline = &getConfiguration()->pipeline;
    gpsConfig = &getConfiguration()->telit->config.globalPositioningSettings;
    
    char splitString[11][16] = {};
    bool validString[11] = {};
    unsigned int i = 0;
    
    // $GPSACP: <UTC>,<latitude>,<longitude>,<hdop>,<altitude>,<fix>,<cog>,<spkm>,<spkn>,<date>,<nsat>
    
    // e.g.: $GPSACP: 080220.479,4542.82691N,01344.26820E,259.07,3,2.1,0.1,0.0,0.0,270705,09 (gps fixed)
    // e.g.: $GPSACP: ,,,,,1,,,,, (gps acquiring satellites)
    // e.g.: $GPSACP: (gps disabled)
    
    // OpenXC will want a set of individual JSON fields like:
     // "latitude" (-89.9999 to +89.9999 degrees)
     // "longitude" (-179.9999 to +179.9999 degrees)
     // "altitude" (n.n meters above MSL)
     // "gps_course" (n.n degrees from True North)
     // "gps_speed" (n.n kph)
     // "gps_nsat" (n)
     // "gps_time" (hhmmss.sss UTC)
     // "gps_date" (ddmmyy)
    
    // split 'er up
    p1 = (char*)GPSACP;
    for(i = 0; i < 10; ++i) {
        if(p2 = strchr(p1, ','), !p2) {
            rc = false;
            return rc;
        }
        memcpy(&splitString[i][0], p1, p2-p1);
        validString[i] = (p2-p1 > 0) ? true : false;
        p1=p2+1;
    }
    if(*p1 > 0x2F && *p1 < 0x3A) {
        memcpy(&splitString[10][0], p1, 2);
        validString[10] = true;
    }
    
    // 'gps_time'
    if(validString[0] && gpsConfig->gpsEnableSignal_gps_time) {
        publishGPSSignal("gps_time", &splitString[0][0], pipeline);
    }
    
    // 'gps_latitude'
    if(validString[1] && gpsConfig->gpsEnableSignal_gps_latitude)  {
        memcpy(tmp, &splitString[1][0], 2); // pull out the degrees (always two digits xx)
        field_value_numerical = (float)atoi(tmp); // turn degrees into float
        field_value_numerical += atof(&splitString[1][0]+2) / 60.0; // get the minutes and convert to degrees
        if(strchr(&splitString[1][0], 'S')) field_value_numerical = -field_value_numerical; // negative sign for S
        publishGPSSignal("gps_latitude", field_value_numerical, pipeline);
    }
    
    // 'gps_longitude'
    if(validString[2] && gpsConfig->gpsEnableSignal_gps_longitude) {
        memcpy(tmp, &splitString[2][0], 3); // pull out the degrees (always three digits xxx)
        field_value_numerical = (float)atoi(tmp); // turn degrees into float
        field_value_numerical += atof(&splitString[2][0]+3) / 60.0; // get the minutes and convert to degrees
        if(strchr(&splitString[2][0], 'W')) field_value_numerical = -field_value_numerical; // negative sign for W
        publishGPSSignal("gps_longitude", field_value_numerical, pipeline);
    }
    
    // 'gps_hdop'
    if(validString[3] && gpsConfig->gpsEnableSignal_gps_hdop) {
        field_value_numerical = atof(&splitString[3][0]);
        publishGPSSignal("gps_hdop", field_value_numerical, pipeline);
    }
    
    // 'gps_altitude'
    if(validString[4] && gpsConfig->gpsEnableSignal_gps_altitude) {
        field_value_numerical = atof(&splitString[4][0]);
        publishGPSSignal("gps_altitude", field_value_numerical, pipeline);
    }
    
    // 'gps_fix'
    if(validString[5] && gpsConfig->gpsEnableSignal_gps_fix) {
        field_value_numerical = (float)atoi(&splitString[5][0]);
        if(field_value_numerical < openxc::telitHE910::FIX_MAX_ENUM)
            publishGPSSignal("gps_fix", (char*)gps_fix_enum[(unsigned int)field_value_numerical], pipeline);
    }
    
    // 'gps_course'
    if(validString[6] && gpsConfig->gpsEnableSignal_gps_course) {
        field_value_numerical = atof(&splitString[6][0]);
        publishGPSSignal("gps_course", field_value_numerical, pipeline);
    }
    
    // 'gps_speed'
    if(validString[7] && gpsConfig->gpsEnableSignal_gps_speed) {
        field_value_numerical = atof(&splitString[7][0]);//* 100;    // there is a bug in the telit firmware that reports speed/100...
        publishGPSSignal("gps_speed", field_value_numerical, pipeline);
    }
    
    // 'gps_speed_knots'
    if(validString[8] && gpsConfig->gpsEnableSignal_gps_speed_knots) {
        field_value_numerical = atof(&splitString[8][0]);//* 100;    // there is a bug in the telit firmware that reports speed/100...
        publishGPSSignal("gps_speed_knots", field_value_numerical, pipeline);
    }
    
    // 'gps_date'
    if(validString[9] && gpsConfig->gpsEnableSignal_gps_date) {
        publishGPSSignal("gps_date", &splitString[9][0], pipeline);
    }
    
    // 'gps_nsat'
    if(validString[10] && gpsConfig->gpsEnableSignal_gps_nsat) {
        field_value_numerical = atof(&splitString[10][0]);
        publishGPSSignal("gps_nsat", field_value_numerical, pipeline);
    }
}

/*PIPELINE*/

/*
 * Public:
 *
 * During pipeline::process(), this function will be called. The TelitDevice contains a QUEUE 
 * that already has unprocessed data from previous calls to pipeline::publish. Here we simply need
 * to flush the send QUEUE through a TCP/IP socket by constructing an HTTP POST. We can do this on 
  * every call, or only when the QUEUE reaches a certain size and/or age.
 */
void openxc::telitHE910::processSendQueue(TelitDevice* device) {

    // The pipeline is made up of a limited 256 byte queue_uint8_t defined in bytebuffer and emqueue
    // The job of a processSendQueue device method is to empty the pipeline
    // Normally we would just dump that queue straight out of the device (UART, USB)
    // Right now, we are reformatting the pipeline data into a JSON array and POSTing via httpClient
    // However, it might become necessary to instead feed a larger buffer that flushes more slowly, 
     // so as to minimize the overhead (both time and data) incurred by the HTTP transactions
    // Thus the QUEUE is buffering data between successive iterations of firmwareLoop(), and 
    // our "sendBuffer" will buffer up multiple QUEUEs before flushing on a time and/or data watermark.

    // pop bytes from the device send queue (stop short of sendBuffer overflow)
    while(!QUEUE_EMPTY(uint8_t, &device->sendQueue) && (pSendBuffer - sendBuffer) < SEND_BUFFER_SIZE) {
        *pSendBuffer++ = QUEUE_POP(uint8_t, &device->sendQueue);
    }

    return;

}

/*
 * Public:
 *
 * Returns number of bytes allocated for the device data send buffer.
 */
unsigned int openxc::telitHE910::sizeSendBuffer(TelitDevice* device) {
    return SEND_BUFFER_SIZE;
 }
 

/*
 * Public:
 *
 * Resets the tracking pointer for the device data send buffer, which resets buffer to empty state.
 */
void openxc::telitHE910::resetSendBuffer(TelitDevice* device) {
    pSendBuffer = sendBuffer;
 }
 
/*
 * Public:
 *
 * Returns number of bytes stored in the device data send buffer.
 */
unsigned int openxc::telitHE910::bytesSendBuffer(TelitDevice* device) {
    return int(pSendBuffer - sendBuffer);
 }
 
/*
 * Public:
 *
 * Copies all bytes from the device data send buffer to the destination pointer, within the limits of the specified length.
 */
unsigned int openxc::telitHE910::readAllSendBuffer(TelitDevice* device, char* destination, unsigned int read_len) {
    unsigned int write_len;
    unsigned int size_buf = bytesSendBuffer(device);
    write_len = (read_len < size_buf) ? read_len : size_buf;
    memcpy(destination, sendBuffer, write_len);
    return write_len;
 }

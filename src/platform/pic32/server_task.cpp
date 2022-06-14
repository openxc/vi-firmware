#include "util/timer.h"
#include "config.h"
#include "payload/payload.h"
#include "telit_he910.h"
#include "server_task.h"
#include "server_apis.h"
#include <stdint.h>

#define GET_FIRMWARE_INTERVAL    600000
#define GET_COMMANDS_INTERVAL    10000
#define POST_DATA_MAX_INTERVAL    5000

using openxc::server_api::serverGETfirmware;
using openxc::server_api::serverPOSTdata;
using openxc::server_api::serverGETcommands;
using openxc::server_api::API_RETURN;
using openxc::util::time::uptimeMs;
using openxc::telitHE910::TelitDevice;
using openxc::telitHE910::isSocketOpen;
using openxc::telitHE910::openSocket;
using openxc::telitHE910::closeSocket;
using openxc::telitHE910::resetSendBuffer;
using openxc::telitHE910::bytesSendBuffer;
using openxc::telitHE910::readAllSendBuffer;
using openxc::server_api::resetCommandBuffer;
using openxc::config::getConfiguration;
using openxc::payload::PayloadFormat;

void openxc::server_task::firmwareCheck(TelitDevice* device) {
    
    static unsigned int state = 0;
    static bool first = true;
    static unsigned long timer = 0xFFFF;

    switch(state)
    {
        default:
            state = 0;
        case 0:
            // check interval if it's not our first time
            if(!first)
            {
                // request upgrade on interval
                if(uptimeMs() - timer > GET_FIRMWARE_INTERVAL)
                {
                    timer = uptimeMs();
                    state = 1;
                }
            }
            else
            {
                first = false;
                timer = uptimeMs();
                state = 1;
            }
            break;
            
        case 1:
            if(!isSocketOpen(GET_FIRMWARE_SOCKET))
            {
                if(!openSocket(GET_FIRMWARE_SOCKET, device->config.serverConnectSettings))
                {
                    state = 0;
                }
                else
                {
                    state = 2;
                }
            }
            else
            {
                state = 2;
            }
            break;
            
        case 2:
            // call the GETfirmware API
            switch(serverGETfirmware(device->deviceId, device->config.serverConnectSettings.host))
            {
                case server_api::None:
                case server_api::Working:
                    // if we are working, do nothing
                    break;
                default:
                case server_api::Success:
                case server_api::Failed:
                    // whether we succeeded or failed (or got lost), there's nothing we can do except go around again
                    // close the socket
                    // either we got a 200 OK, in which case we went for reset
                    // or we got a 204/error, in which case we have a dangling transaction
                    closeSocket(GET_FIRMWARE_SOCKET);
                    //timer = uptimeMs();
                    state = 0;
                    break;
                //default: //Do nothing by default
                    //break;
            }
            break;
    }

    return;
    
}

void openxc::server_task::flushDataBuffer(TelitDevice* device) {

    static bool first = true;
    static unsigned int state = 0;
    static unsigned int lastFlushTime = 0;
    static const unsigned int flushSize = 2048;
    static char postBuffer[SEND_BUFFER_SIZE + 64]; // extra space needed for root record
    static unsigned int byteCount = 0;
    unsigned int i = 0;
    static unsigned int bufSize = 0;
    
    switch(state)
    {
        default:
            state = 0;
        case 0:
            // conditions to flush the outgoing data buffer
                // a) buffer has reached the flush size
                // b) buffer has not been flushed for the time period POST_DATA_MAX_INTERVAL (and there is something in there)
                // c) minimum amount of time has passed since lastFlushTime (depends on socket status) - not yet implemented
            if(!first)
            {
                bufSize = bytesSendBuffer(device);
                if( (bufSize >= flushSize) || 
                    ((uptimeMs() - lastFlushTime >= POST_DATA_MAX_INTERVAL) && (bufSize > 0)) )
                {
                    lastFlushTime = uptimeMs();
                    state = 1;
                }
            }
            else
            {
                first = false;
                lastFlushTime = uptimeMs();
                state = 1;
            }
            break;
            
        case 1:
            // ensure we have an open TCP/IP socket
            if(!isSocketOpen(POST_DATA_SOCKET))
            {
                if(!openSocket(POST_DATA_SOCKET, device->config.serverConnectSettings))
                {
                    state = 0;
                }
                else
                {
                    state = 2;
                }
            }
            else
            {
                state = 2;
            }
            break;
            
        case 2:
            
            switch(getConfiguration()->payloadFormat)
            {
                case PayloadFormat::JSON:
                
                    // pre-populate the send buffer with root record
                    memcpy(postBuffer, "{\"records\":[", 12);
                    byteCount = 12;
                    
                    // get all bytes from the send buffer (so we have room to fill it again as we POST)
                    byteCount += readAllSendBuffer(device, postBuffer+byteCount, sizeof(postBuffer) - byteCount);
                    resetSendBuffer(device);
                    
                    // replace the nulls with commas to create a JSON array
                    for(i = 0; i < byteCount; ++i)
                    {
                        if(postBuffer[i] == '\0')
                            postBuffer[i] = ',';
                    }
                    
                    // back over the trailing comma
                    if(postBuffer[byteCount-1] == ',')
                        byteCount--;
                    
                    // end the array
                    postBuffer[byteCount++] = ']';
                    postBuffer[byteCount++] = '}';
                
                    break;
                    
                case PayloadFormat::PROTOBUF:
                
                    // get all bytes from the send buffer (so we have room to fill it again as we POST)
                    byteCount = 0;
                    byteCount += readAllSendBuffer(device, postBuffer+byteCount, sizeof(postBuffer) - byteCount);
                    resetSendBuffer(device);
                
                    break;
            }
            
            state = 3;
            
            break;
            
        case 3:
        
            // call the POSTdata API
            switch(serverPOSTdata(device->deviceId, device->config.serverConnectSettings.host, postBuffer, byteCount))
            {
                case server_api::None:
                case server_api::Working:
                    // if we are working, do nothing
                    break;
                default:
                case server_api::Success:
                    state = 0;
                    break;
                case server_api::Failed:
                    //lastFlushTime = uptimeMs();
                    state = 0;
                    closeSocket(POST_DATA_SOCKET);
                    break;
                //default: //Do nothing by default
                    //break;
            }
            break;
    }
    
    return;

}

void openxc::server_task::commandCheck(TelitDevice* device) {

    static unsigned int state = 0;
    static bool first = true;
    static unsigned long timer = 0xFFFF;
    uint8_t* pCommand = NULL;
    unsigned int cmd_len = 0;

    switch(state)
    {
        default:
            state = 0;
        case 0:
            // check interval if it's not our first time
            if(!first)
            {
                // request commands on interval
                if(uptimeMs() - timer > GET_COMMANDS_INTERVAL)
                {
                    timer = uptimeMs();
                    state = 1;
                }
            }
            else
            {
                first = false;
                timer = uptimeMs();
                state = 1;
            }
            break;
            
        case 1:
            if(!isSocketOpen(GET_COMMANDS_SOCKET))
            {
                if(!openSocket(GET_COMMANDS_SOCKET, device->config.serverConnectSettings))
                {
                    state = 0;
                }
                else
                {
                    state = 2;
                }
            }
            else
            {
                state = 2;
            }
            break;
            
        case 2:
            // call the GETconfigure API
            switch(serverGETcommands(device->deviceId, device->config.serverConnectSettings.host, &pCommand, &cmd_len))
            {
                case server_api::None:
                case server_api::Working:
                    // if we are working, do nothing
                    break;
                default:
                case server_api::Success:
                    if((pCommand != NULL) && (cmd_len > 0))
                    {
                        // send the contents of commandBuffer to the command handler
                        commands::handleIncomingMessage(pCommand, cmd_len, &( device->descriptor ) );
                    }
                    resetCommandBuffer();
                    state = 0;
                    break;
                case server_api::Failed:
                    resetCommandBuffer();
                    closeSocket(GET_COMMANDS_SOCKET);
                    state = 0;
                    break;
            }
            break;
    }

    return;

}

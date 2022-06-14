#include "http.h"
#include "util/log.h"
#include "util/timer.h"
#include "platform/pic32/telit_he910.h"
#include <stdio.h>

namespace http = openxc::http;

using openxc::util::time::delayMs;
using openxc::util::log::debug;
using openxc::util::time::uptimeMs;
using openxc::http::httpClient;
using openxc::http::HTTP_STATUS;

/*HTTP*/

/*
 * This is a VERY lightweight HTTP client. It is meant to handle the basics 
 * of executing a generic HTTP transaction. The specifics of the transaction 
 * (i.e. request method and URI, response handling) are left to the caller.
 *
 * Uses the open source 'http-parser' (https://github.com/joyent/http-parser) to 
 * parse the HTTP response and pass back data.
 *
 * The caller is responsible for:
 *  - opening and closing the network socket (gives flexibility to leave it open,
 *   w/o having to invoke an HTTP_REQUEST parser to look for the 'Connection:' field)
 *  - defining the HTTP method to be used (GET, POST, PUT....)
 *  - composing and providing a pointer to the fully-formed HTTP request header
 *  - composing and providing a pointer to the HTTP request body data
 *  - receiving and handling the entire HTTP response body (in chunks as determined by the client)
 * The HTTP client is responsible for:
 *  - composing and transmitting a complete HTTP request via the socket send callback
 *  - receiving a complete HTTP response (in chunks) via the socket read callback
 *  - parsing the HTTP response header and extracting basic information such as response code (200, 404...)
 *  - returning some basic HTTP response info and the HTTP response body to the caller (in chunks) via the 'put' callback
 */
 
#define HTTP_TIMEOUT_PERIOD        10000
#define HTTP_CHECK_RESPONSE_DELAY    500

static httpClient* gContext;

static int http_parser_cb_on_message_begin(http_parser* parser);
static int http_parser_cb_on_url(http_parser* parser, const char *at, size_t length);
static int http_parser_cb_on_status(http_parser* parser, const char *at, size_t length);
static int http_parser_cb_on_header_field(http_parser* parser, const char *at, size_t length);
static int http_parser_cb_on_header_value(http_parser* parser, const char *at, size_t length);
static int http_parser_cb_on_headers_complete(http_parser* parser);
static int http_parser_cb_on_body(http_parser* parser, const char *at, size_t length);
static int http_parser_cb_on_message_complete(http_parser* parser);

static int http_parser_cb_on_message_begin(http_parser* parser) {
    //debug("On Message Begin Callback!"); 
    return 0;
}

static int http_parser_cb_on_url(http_parser* parser, const char *at, size_t length) {
    //debug("On URL Callback! Data: %.*s", length, at); 
    return 0;
}

static int http_parser_cb_on_status(http_parser* parser, const char *at, size_t length) {
    //debug("On Status Callback! Data: %.*s", length, at);
    return 0;
}

static int http_parser_cb_on_header_field(http_parser* parser, const char *at, size_t length) {
    //debug("Header Field Callback! Data: %.*s", length, at); 
    return 0;
}

static int http_parser_cb_on_header_value(http_parser* parser, const char *at, size_t length) {
    //debug("Header Value Callback! Data: %.*s", length, at); 
    return 0;
}

static int http_parser_cb_on_headers_complete(http_parser* parser) {
    //debug("Headers Complete Callback!"); 
    if(gContext) {
        gContext->responseCode = parser->status_code;
    }
    return 0;
}

static int http_parser_cb_on_body(http_parser* parser, const char *at, size_t length) {
    //debug("On Body Callback! Data: %.*s", length, at); 
    return 0;
}

static int http_parser_cb_on_message_complete(http_parser* parser) {
    //debug("On Message Complete Callback!"); 
    if(gContext) {
        gContext->responseComplete = true; 
    }
    return 0;
}

// class constructor
httpClient::httpClient() {

    timer = 0;

    socketNumber = 1;
    status = HTTP_READY;
    bytesSent = 0;
    bytesReceived = 0;
    
    requestHeaderSize = 0;
    requestBodySize = 0;
    requestHeader = NULL;
    requestBody = NULL;
    
    responseHeaderSize = 0;
    responseBodySize = 0;
    responseCode = 0;
    memset(responseData, 0x00, bufferSize);
    responseComplete = false;
    
    // http-parser callbacks
    parser_settings.on_message_begin = &http_parser_cb_on_message_begin;
    parser_settings.on_url = &http_parser_cb_on_url;
    parser_settings.on_status = &http_parser_cb_on_status;
    parser_settings.on_header_field = &http_parser_cb_on_header_field;
    parser_settings.on_header_value = &http_parser_cb_on_header_value;
    parser_settings.on_headers_complete = &http_parser_cb_on_headers_complete;
    parser_settings.on_body = &http_parser_cb_on_body;
    parser_settings.on_message_complete = &http_parser_cb_on_message_complete;
    
    // default response data callback (do nothing)
    cbPutResponseData = &httpClient::cbDefault;
}

HTTP_STATUS httpClient::execute() {
 
    switch(status) {
        case HTTP_READY:
        
            // activate client
            gContext = this;
            
            // validate client parameters
            if(!requestHeader || !sendSocketData || !isReceiveDataAvailable || !receiveSocketData) {
                status = HTTP_FAILED;
                break;
            }
            
            startTime = uptimeMs();
            requestHeaderSize = strlen(requestHeader);
            http_parser_init(&parser, HTTP_RESPONSE);
            status = HTTP_SENDING_REQUEST_HEADER;
            
        case HTTP_SENDING_REQUEST_HEADER:
        
            byteCount = requestHeaderSize - bytesSent;
            if(sendSocketData(socketNumber, requestHeader, &byteCount)) {
                requestHeader += byteCount;
                bytesSent += byteCount;
                if(bytesSent >= requestHeaderSize) {
                    status = HTTP_SENDING_REQUEST_BODY;
                }
            }
            else {
                status = HTTP_FAILED;
            }
            
            break;
            
        case HTTP_SENDING_REQUEST_BODY:
        
            if(!requestBody) {
                status = HTTP_RECEIVING_RESPONSE;
            }
            else {
                byteCount = requestBodySize - (bytesSent - requestHeaderSize);
                if(sendSocketData(socketNumber, requestBody, &byteCount)) {
                    requestBody += byteCount;
                    bytesSent += byteCount;
                    if(bytesSent - requestHeaderSize >= requestBodySize) {
                        status = HTTP_RECEIVING_RESPONSE;
                    }
                }
                else {
                    status = HTTP_FAILED;
                }
            }
            
            break;
            
        case HTTP_RECEIVING_RESPONSE:
        
            timer = uptimeMs();
        
            if(isReceiveDataAvailable(socketNumber)) {
                byteCount = bufferSize;
                if(receiveSocketData(socketNumber, responseData, &byteCount)) {
                    bytesReceived += byteCount;
                    parser.data = responseData;
                    if(http_parser_execute(&parser, &parser_settings, responseData, byteCount) != byteCount) {
                        status = HTTP_FAILED;
                        break;
                    }
                    if(responseComplete) {
                        status = HTTP_COMPLETE;
                    }
                    startTime = uptimeMs();
                }
            }
            else {
                status = HTTP_WAIT;
            }
            
            break;
            
        case HTTP_WAIT:
            if(uptimeMs() - timer > HTTP_CHECK_RESPONSE_DELAY)
                status = HTTP_RECEIVING_RESPONSE;
            break;
            
        case HTTP_COMPLETE:
            goto fcn_exit;
            break;
            
        case HTTP_FAILED:
            goto fcn_exit;
            break;

        //default: //Do nothing by default
            //break;
    }
    
    // watch for timeout
    if(uptimeMs() - startTime > HTTP_TIMEOUT_PERIOD) {
        status = HTTP_FAILED;
    }
    
    fcn_exit:
    return status;
 
 }

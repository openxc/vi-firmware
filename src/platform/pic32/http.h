#ifndef _HTTP_H_
#define _HTTP_H_

#include "libs/http-parser/http_parser.h"
#include <stdio.h>

#define HTTP_BUFFERSIZE        1024

namespace openxc {
namespace http {

typedef enum {
    HTTP_READY,
    HTTP_SENDING_REQUEST_HEADER,
    HTTP_SENDING_REQUEST_BODY,
    HTTP_RECEIVING_RESPONSE,
    HTTP_WAIT,
    HTTP_COMPLETE,
    HTTP_FAILED,
} HTTP_STATUS;

class httpClient {
 
    private:
    
        // buffer size
        static const unsigned int bufferSize = HTTP_BUFFERSIZE;
        // generic timer
        unsigned int timer;
    
    public:
        // http parser
        http_parser_settings parser_settings;    // settings for parsing http data
        http_parser parser;                        // parser for http data
        
        // do nothing (default) callback
        static void cbDefault(char*, unsigned int){}
 
        // transaction details
        unsigned int socketNumber;
        HTTP_STATUS status;                    // status of the http client state machine
        unsigned int startTime;                // start time of this transaction (used to time out a hanging transaction)
        unsigned int bytesSent;                // total bytes sent in this transaction
        unsigned int bytesReceived;            // total bytes received in this transaction
        unsigned int byteCount;                // a general byte counter for the transaction
        
        // request details
        char* requestHeader;                // pointer to request header 
        unsigned int requestHeaderSize;        // set by strlen(requestHeader) during initialization when we get the ptr
        char* requestBody;                    // pointer to request body -- could replace with the cbGetRequestData() callback
        unsigned int requestBodySize;        // passed during initialization
        
        // response details
        bool responseComplete;                // used to signal that http-parser is finished
        unsigned int responseHeaderSize;    // not really used
        unsigned int responseBodySize;        // not really used
        unsigned int responseCode;            // HTTP status code (numerical)
        char responseData[bufferSize];        // place to read socket data into for parsing
        
        // data callbacks
        bool (*sendSocketData)(unsigned int, char*, unsigned int*);            // callback used to send data to server
        bool (*isReceiveDataAvailable)(unsigned int);                        // callback to find out if there is data available from server
        bool (*receiveSocketData)(unsigned int, char*, unsigned int*);        // callback to receive data from server
        void (*cbGetRequestData)(char*, unsigned int);
        void (*cbPutResponseData)(char*, unsigned int);                        // callback to handle server response data
        void (*cbHeaderComplete)();
        
        // constructor
        httpClient();
        
        // execute http client
        HTTP_STATUS execute();
    
};

} // namespace http
} // namespace openxc

 #endif // _HTTP_H_

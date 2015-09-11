#include "server_apis.h"
#include "telit_he910.h"
#include "http.h"

/*SOCKET ASSIGNMENTS*/

#define GET_FIRMWARE_SOCKET		1
#define POST_DATA_SOCKET		2
#define GET_COMMANDS_SOCKET		3

/*PRIVATE FUNCTION DECLARATIONS*/

static int cbOnBody(http_parser* parser, const char* at, size_t length);
static int cbOnStatus(http_parser* parser, const char* at, size_t length);
static int cbHeaderComplete(http_parser* parser);

/*API CALLS (PUBLIC)*/

API_RETURN openxc::server_api::serverPOSTdata(char* deviceId, char* host, char* data, unsigned int len) {

	static API_RETURN ret = None;
	static http::httpClient client;
	static char header[256];
	static unsigned int state = 0;
	static const char* ctJSON = "application/json";
	static const char* ctPROTOBUF = "application/x-protobuf";
	
	switch(state)
	{
		default:
			state = 0;
		case 0:
			ret = Working;
			// compose the header for POST /data
			sprintf(header, "POST /api/%s/data HTTP/1.1\r\n"
					"Content-Length: %u\r\n"
					"Content-Type: %s\r\n"
					"Host: %s\r\n"
					"Connection: Keep-Alive\r\n\r\n", deviceId, len, getConfiguration()->payloadFormat == PayloadFormat::PROTOBUF ? ctPROTOBUF : ctJSON, host);
			// configure the HTTP client
			client = http::httpClient();
			client.socketNumber = POST_DATA_SOCKET;
			client.requestHeader = header;
			client.requestBody = data;
			client.requestBodySize = len;
			client.cbGetRequestData = NULL;
			client.cbPutResponseData = NULL;
			client.sendSocketData = &openxc::telitHE910::writeSocket;
			client.isReceiveDataAvailable = &openxc::telitHE910::isSocketDataAvailable;
			client.receiveSocketData = &openxc::telitHE910::readSocket;
			state = 1;
			break;
			
		case 1:
			// run the HTTP client
			switch(client.execute())
			{
				case http::HTTP_READY:
				case http::HTTP_SENDING_REQUEST_HEADER:
				case http::HTTP_SENDING_REQUEST_BODY:
				case http::HTTP_RECEIVING_RESPONSE:
					// nothing to do while client is in progress
					break;
				case http::HTTP_COMPLETE:
					ret = Success;
					state = 0;
					break;
				case http::HTTP_FAILED:
					ret = Failed;
					state = 0;
					break;
			}
			break;
	}
	
	return ret;

}

API_RETURN openxc::server_api::serverGETfirmware(char* deviceId, char* host) {

	static API_RETURN ret = None;
	static http::httpClient client;
	static char header[256];
	static unsigned int state = 0;
	
	switch(state)
	{
		default:
			state = 0;
		case 0:
			ret = Working;
			// compose the header for GET /firmware
			sprintf(header, "GET /api/%s/firmware HTTP/1.1\r\n"
					"If-None-Match: \"%s\"\r\n"
					"Host: %s\r\n"
					"Connection: Keep-Alive\r\n\r\n", deviceId, getConfiguration()->flashHash, host);
			// configure the HTTP client
			client = http::httpClient();
			client.socketNumber = GET_FIRMWARE_SOCKET;
			client.requestHeader = header;
			client.requestBody = NULL;
			client.requestBodySize = 0;
			client.cbGetRequestData = NULL;
			client.parser_settings.on_headers_complete = &cbHeaderComplete;
			client.parser_settings.on_status = &cbOnStatus;
			client.cbPutResponseData = NULL;
			client.sendSocketData = &openxc::telitHE910::writeSocket;
			client.isReceiveDataAvailable = &openxc::telitHE910::isSocketDataAvailable;
			client.receiveSocketData = &readSocketOne;
			state = 1;
			break;
			
		case 1:
			// run the HTTP client
			switch(client.execute())
			{
				case http::HTTP_READY:
				case http::HTTP_SENDING_REQUEST_HEADER:
				case http::HTTP_SENDING_REQUEST_BODY:
				case http::HTTP_RECEIVING_RESPONSE:
					// nothing to do while client is in progress
					break;
				case http::HTTP_COMPLETE:
					ret = Success;
					state = 0;
					break;
				case http::HTTP_FAILED:
					ret = Failed;
					state = 0;
					break;
			}
			break;
	}
	
	return ret;
	
}

API_RETURN openxc::server_api::serverGETcommands(char* deviceId, char* host) {
	
	static API_RETURN ret = None;
	static http::httpClient client;
	static char header[256];
	static unsigned int state = 0;
	
	switch(state)
	{
		default:
			state = 0;
		case 0:
			ret = Working;
			// compose the header for GET /firmware
			sprintf(header, "GET /api/%s/configure HTTP/1.1\r\n"
					"Host: %s\r\n"
					"Connection: Keep-Alive\r\n\r\n", deviceId, host);
			// configure the HTTP client
			client = http::httpClient();
			client.socketNumber = GET_COMMANDS_SOCKET;
			client.requestHeader = header;
			client.requestBody = NULL;
			client.requestBodySize = 0;
			client.cbGetRequestData = NULL;
			client.parser_settings.on_body = cbOnBody;
			client.cbPutResponseData = NULL;
			client.sendSocketData = &openxc::telitHE910::writeSocket;
			client.isReceiveDataAvailable = &openxc::telitHE910::isSocketDataAvailable;
			client.receiveSocketData = &openxc::telitHE910::readSocket;
			state = 1;
			break;
			
		case 1:
			// run the HTTP client
			switch(client.execute())
			{
				case http::HTTP_READY:
				case http::HTTP_SENDING_REQUEST_HEADER:
				case http::HTTP_SENDING_REQUEST_BODY:
				case http::HTTP_RECEIVING_RESPONSE:
					// nothing to do while client is in progress
					break;
				case http::HTTP_COMPLETE:
					ret = Success;
					state = 0;
					break;
				case http::HTTP_FAILED:
					ret = Failed;
					state = 0;
					break;
			}
			break;
	}
	
	return ret;
	
}

/*HTTP CALLBACKS (PRIVATE)*/

static int cbOnBody(http_parser* parser, const char* at, size_t length) {
	unsigned int i = 0;
	for(i = 0; i < length && pCommandBuffer < (commandBuffer + commandBufferSize); ++i)
	{
		*pCommandBuffer++ = *at++;
	}
	return 0;
}

static int cbOnStatus(http_parser* parser, const char* at, size_t length) {
	if(parser->status_code == 204)
		return 1; // cause parser to exit immediately (we're throwing an error to force the client to abort)
	else
		return 0;
}

static int cbHeaderComplete(http_parser* parser) {
	if(parser->status_code == 200)
		SoftReset();
	else
		return 1;
}
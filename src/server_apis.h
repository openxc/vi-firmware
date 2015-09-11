namespace openxc {
namespace server_api{

// typedef for SERVER API return codes
typedef enum {
	None,
	Working,
	Success,
	Failed
} API_RETURN;

// external functions
API_RETURN serverPOSTdata(char* deviceId, char* host, char* data, unsigned int len);
API_RETURN serverGETfirmware(char* deviceId, char* host);
API_RETURN serverGETcommands(char* deviceId, char* host);

}
}
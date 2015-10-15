#include <stdint.h>

/*SOCKET ASSIGNMENTS*/

#define GET_FIRMWARE_SOCKET		1
#define POST_DATA_SOCKET		2
#define GET_COMMANDS_SOCKET		3

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
API_RETURN serverGETcommands(char* deviceId, char* host, uint8_t** result, unsigned int* len);
void resetCommandBuffer(void);
unsigned int bytesCommandBuffer(void);

}
}
#include "telit_HE910.h"

namespace openxc {
namespace server_task {
	
void openxc::server_task::firmwareCheck(TelitDevice* device);
void openxc::server_task::flushDataBuffer(TelitDevice* device);
void openxc::server_task::commandCheck(TelitDevice* device);
	
}
}
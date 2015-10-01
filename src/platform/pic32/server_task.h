#include "telit_he910.h"

namespace openxc {
namespace server_task {

void firmwareCheck(telitHE910::TelitDevice* device);
void flushDataBuffer(telitHE910::TelitDevice* device);
void commandCheck(telitHE910::TelitDevice* device);

}
}

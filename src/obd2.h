#ifndef __OBD2_H__
#define __OBD2_H__

#include <stdint.h>
#include <stdlib.h>
#include "util/timer.h"
#include "diagnostics.h"

namespace openxc {
namespace diagnostics {
namespace obd2 {

void loop(DiagnosticsManager* manager, CanBus* bus);

void initialize(DiagnosticsManager* manager, CanBus* bus);

} // namespace obd2
} // namespace diagnostics
} // namespace openxc

#endif // __OBD2_H__

#ifndef __OBD2_H__
#define __OBD2_H__

#include <stdint.h>
#include <stdlib.h>
#include "util/timer.h"
#include "diagnostics.h"

namespace openxc {
namespace diagnostics {
namespace obd2 {

typedef struct {
    uint8_t pid;
    const char* name;
    float frequency;
} Obd2Pid;

void loop(DiagnosticsManager* manager);

void initialize(DiagnosticsManager* manager);

} // namespace obd2
} // namespace diagnostics
} // namespace openxc

#endif // __OBD2_H__

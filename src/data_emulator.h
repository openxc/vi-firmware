#ifndef __DATA_EMULATOR_H__
#define __DATA_EMULATOR_H__

#include "pipeline.h"

namespace openxc {
namespace emulator {

/* Public: Generate and inject fake vehicle data into the Pipeline.
 *
 * This is useful to test general connectivity with a VI on a bench without
 * having a real vehicle.
 */
void generateFakeMeasurements(openxc::pipeline::Pipeline* pipeline);

void restart();

} // namespace emulator
} // namespace openxc

#endif // __DATA_EMULATOR_H__

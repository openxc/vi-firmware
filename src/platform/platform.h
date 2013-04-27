#ifndef __PLATFORM_H__
#define __PLATFORM_H__

namespace openxc {
namespace platform {

/* Public: Perform any one-time, up front initialization required fro the
 * platform the firmware is running on.
 */
void initializePlatform();

} // namespace platform
} // namespace openxc

#endif // __PLATFORM_H__

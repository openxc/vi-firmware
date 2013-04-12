#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Public: Perform any one-time, up front initialization required fro the
 * platform the firmware is running on.
 */
void initializePlatform();

#ifdef __cplusplus
}
#endif

#endif // __PLATFORM_H__

#ifndef _STRUTIL_H_
#define _STRUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// Microchip Ethernet library comprises a function with
// the same name and the same functionality "strnchr".
// Therefore this section should not be compiled when
// the Ethernet library is included.
#if defined(NO_ETHERNET) || !defined(__PIC32__)
/*
 * Thanks to https://gist.github.com/855214.
 */
const char *strnchr(const char *str, size_t len, char character);

#ifdef __cplusplus
}
#endif

#endif

#endif // _STRUTIL_H_

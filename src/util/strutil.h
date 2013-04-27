#ifndef _STRUTIL_H_
#define _STRUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// Microchip Network library comprises a function with
// the same name and the same functionality "strnchr".
// Therefore this section should not be compiled when
// the Network library is included.
#ifndef __USE_NETWORK__

/*
 * Thanks to https://gist.github.com/855214.
 */
const char *strnchr(const char *str, size_t len, char character);

#ifdef __cplusplus
}
#endif

#endif // __USE_NETWORK__

#endif // _STRUTIL_H_

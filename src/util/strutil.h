#ifndef _STRUTIL_H_
#define _STRUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/*
 * Thanks to https://gist.github.com/855214.
 */
const char *strnchr(const char *str, size_t len, char character);

#ifdef __cplusplus
}
#endif

#endif // _STRUTIL_H_

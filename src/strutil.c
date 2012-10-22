// Microchip Ethernet library comprises a function with
// the same name and the same functionality "strnchr".
// Therefore this section should not be compiled when
// the Ethernet library is included.
#ifdef NO_ETHERNET

#include "strutil.h"

const char *strnchr(const char *str, size_t len, char character) {
    const char *end = str + len;
    do {
        if(*str == character) {
            return str;
        }
    } while (++str <= end);
    return NULL;
}

#endif

#include "strutil.h"

// Microchip Network library comprises a function with
// the same name and the same functionality "strnchr".
// Therefore this section should not be compiled when
// the Network library is included.
#if !(defined(__PIC32__) && defined(__USE_NETWORK__))

const char *strnchr(const char *str, size_t len, char character) {
    const char *end = str + len;
    do {
        if(*str == character) {
            return str;
        }
    } while (++str <= end);
    return NULL;
}

#endif // __USE_NETWORK__

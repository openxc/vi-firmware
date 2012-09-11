#include "strutil.h"

const char *strnchr(const char *str, size_t len, int character) {
    const char *end = str + len;
    char c = (char)character;
    do {
        if(*str == c) {
            return str;
        }
    } while (++str <= end);
    return NULL;
}

#include "bitfield.h"

/**
 * Find the ending bit of a bitfield within the final byte.
 *
 * Returns: a bit position from 0 to 7.
 */
int findEndBit(int startBit, int numBits) {
    int endBit = (startBit + numBits) % 8;
    return endBit == 0 ? 8 : endBit;
}

uint64_t bitmask(int numBits) {
    return (0x1 << numBits) - 1;
}

int startingByte(int startBit) {
    return startBit / 8;
}

int endingByte(int startBit, int numBits) {
    return (startBit + numBits - 1) / 8;
}

uint64_t openxc::util::bitfield::getBitField(uint64_t data, int startBit, int numBits, bool bigEndian) {
    int startByte = startingByte(startBit);
    int endByte = endingByte(startBit, numBits);

    if(!bigEndian) {
        data = __builtin_bswap64(data);
    }
    uint8_t* bytes = (uint8_t*)&data;
    uint64_t ret = bytes[startByte];
    if(startByte != endByte) {
        // The lowest byte address contains the most significant bit.
        int i;
        for(i = startByte + 1; i <= endByte; i++) {
            ret = ret << 8;
            ret = ret | bytes[i];
        }
    }

    ret >>= 8 - findEndBit(startBit, numBits);
    return ret & bitmask(numBits);
}

/**
 * TODO it would be nice to have a warning if you call with this a value that
 * won't fit in the number of bits you've specified it should use.
 */
void openxc::util::bitfield::setBitField(uint64_t* data, uint64_t value, int startBit, int numBits) {
    int shiftDistance = 64 - startBit - numBits;
    value <<= shiftDistance;
    *data &= ~(bitmask(numBits) << shiftDistance);
    *data |= value;
}

uint8_t openxc::util::bitfield::nthByte(uint64_t source, int byteNum) {
    return (source >> (64 - ((byteNum + 1) * 8))) & 0xFF;
}

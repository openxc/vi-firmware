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

unsigned long bitmask(int numBits) {
    return (unsigned long)((0x1 << numBits) - 1);
}

unsigned long reverseBitmaskVariableLength(int numBits, int totalLength) {
    unsigned long mask = bitmask(numBits);
    return mask << totalLength - numBits;
}

unsigned long reverseBitmask(int numBits) {
    return reverseBitmaskVariableLength(numBits, 32);
}


int startingByte(int startBit) {
    return startBit / 8;
}

int endingByte(int startBit, int numBits) {
    return (startBit + numBits - 1) / 8;
}

unsigned long getBitField(uint8_t* data, int startBit, int numBits) {
    int startByte = startingByte(startBit);
    int endByte = endingByte(startBit, numBits);

    unsigned long ret = data[startByte];
    if(startByte != endByte) {
        // The lowest byte address contains the most significant bit.
        for (int i = startByte + 1; i <= endByte; i++) {
            ret = ret << 8;
            ret = ret | data[i];
        }
    }

    ret >>= 8 - findEndBit(startBit, numBits);
    return ret & bitmask(numBits);
}

/**
 * TODO it would be nice to have a warning if you call with this a value that
 * won't fit in the number of bits you've specified it should use.
 *
 * TODO document this and all of the byte ordering in a sane fashion.
 */
void setBitField(uint8_t* data, unsigned long value, int startBit,
        int numBits) {
    int startByte = startingByte(startBit);
    int endByte = endingByte(startBit, numBits);
    // TODO need to slide over numbits, I think. but this should work with
    // values ending on the byte boundary
    int numBytes = endByte - startByte + 1;
    for(int i = startByte; i <= endByte; i++) {
        uint8_t block = (value >> ((numBytes * 8) -
                    (8 * (i - startByte + 1)))) & bitmask(8);
        data[i] = (data[i] | bitmask(8)) & block;
    }
    data[endByte] <<= 8 - findEndBit(startBit, numBits);
}


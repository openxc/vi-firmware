#include "bitfield.h"

/**
 * Find the ending bit of a bitfield within the final byte.
 *
 * Returns: a bit position from 0 to 7.
 */
int findEndBit(int startBit, int numBits) {
    if (numBits <= 8 ) {
        // Bit fields are positioned according to big-endian bit layout, but
        // inside the bit field, values are represented as little-endian.
        // Therefore, to get the bit field, we just need to convert to big-endian
        // bit ordering to find the field, and directly use the value we find in
        // the field.
        return (startBit % 8) + numBits;
    } else {
        // Calculates value to shift bitfield of interest to LSB
        return (startBit + numBits) % 8;
    }
}

unsigned long getBitField(uint8_t* data, int startBit, int numBits) {
    unsigned long ret = 0;

    int startByte = startBit / 8;
    int endByte = (startBit + numBits - 1) / 8;

    ret = data[startByte];
    if(startByte != endByte) {
        // The lowest byte address contains the most significant bit.
        for (int i = startByte + 1; i <= endByte; i++) {
            ret = ret << 8;
            ret = ret | data[i];
        }
    }

    ret = ret >> (8 - findEndBit(startBit, numBits));

    // Mask out any other bits besides those in the bitfield.
    unsigned long bitmask = (unsigned long)((0x1 << numBits) - 1);
    return ret & bitmask;
}

void setBitField(uint8_t* data, float value, int startPos) {
    // TODO
}

#include "bitfield.h"

unsigned long getBitField(uint8_t* data, int startBit, int numBits) {
    unsigned long ret = 0;

    int startByte = startBit / 8;

    if (numBits <= 8 ) {
        // Bit fields are positioned according to big-endian bit layout, but
        // inside the bit field, values are represented as little-endian.
        // Therefore, to get the bit field, we just need to convert to big-endian
        // bit ordering to find the field, and directly use the value we find in
        // the field.
        int bitPosition = startBit % 8;
        ret = data[startByte];
        int endBit = bitPosition + numBits;
        ret = ret >> (8 - endBit);
    } else {
        int endByte = (startBit + numBits) / 8;

        // The lowest byte address contains the most significant bit.
        for (int i = startByte; i <= endByte; i++) {
            ret = ret << 8;
            ret = ret | data[i];
        }

        //Calculates value to shift bitfield of interest to LSB
        ret = ret >> (8 - ((startBit + numBits) % 8));
    }

    // Mask out any other bits besides those in the bitfield.
    unsigned long bitmask = (unsigned long)((0x1 << numBits) - 1);
    return ret & bitmask;
}

void setBitField(uint8_t* data, float value, int startPos) {
    // TODO
}

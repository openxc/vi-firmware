#include "bitfield.h"

unsigned long get_bit_field(uint8_t* data, int startPos, int numBits) {
  unsigned long ret = 0;

  // Calculate the starting byte.
  int startByte = startPos / 8;
  // Mask out any other bits besides those in the bitfield.
  unsigned long bitmask = (unsigned long)((0x1 << numBits) - 1);

  if (numBits <= 8 ) {
    //Calculate the starting bit position

    int startBit = startPos % 8;
    ret = data[startByte];

    // New method: bit fields are positioned according to big-endian
    // bit layout, but inside the bit field, values are represented
    // as little-endian.
    // Therefore, to get the bit field, we just need to convert to big-endian
    // bit ordering to find the field, and directly use the value we find in
    // the field.
    // Calculate the end bit address of the field
    int endBit = startBit + numBits;
    ret = (ret >> (8 - endBit));
    ret = (ret & bitmask);
  } else {
    int endByte = (startPos + numBits) / 8;

    // The lowest byte address contains the most significant bit.
    for (int i = startByte; i < endByte; i++) {
	ret = ret << 8;
	ret = ret | data[i];
      }
    ret = ret & bitmask;
  }
  return ret;
}

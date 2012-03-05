#include "bitfield.h"

unsigned long getBitField(uint8_t* data, int start_bit, int num_bits) {
  unsigned long ret = 0;

  int start_byte = start_bit / 8;

  if (num_bits <= 8 ) {
      // Bit fields are positioned according to big-endian bit layout, but
      // inside the bit field, values are represented as little-endian.
      // Therefore, to get the bit field, we just need to convert to big-endian
      // bit ordering to find the field, and directly use the value we find in
      // the field.
      int bit_position = start_bit % 8;
      ret = data[start_byte];
      int end_bit = bit_position + num_bits;
      ret = ret >> (8 - end_bit);
  } else {
      int end_byte = (start_bit + num_bits) / 8;

      // The lowest byte address contains the most significant bit.
      for (int i = start_byte; i <= end_byte; i++) {
          ret = ret << 8;
          ret = ret | data[i];
      }

      //Calculates value to shift bitfield of interest to LSB
      ret = ret >> (8 - ((start_bit + num_bits) % 8));
  }

  // Mask out any other bits besides those in the bitfield.
  unsigned long bitmask = (unsigned long)((0x1 << num_bits) - 1);
  return ret & bitmask;
}

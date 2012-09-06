#ifndef _BITFIELD_H_
#define _BITFIELD_H_

#include <stdint.h>

/* Public: Reads a subset of bits from a byte array.
 *
 * data - the bytes in question.
 * startPos - the starting index of the bit field (beginning from 0).
 * numBits - the width of the bit field to extract.
 *
 * Bit fields are positioned according to big-endian bit layout, but inside the
 * bit field, values are represented as little-endian. Therefore, to get the bit
 * field, we just need to convert to big-endian bit ordering to find the field,
 * and directly use the value we find in the field.
 *
 * For example, the bit layout of the value "42" (i.e. 00101010 set at position
 * 14 with length 6 is:
 *
 *     000000000000001010100000000000000000000000000000000000000000000
 *
 * and the same value and position but with length 8 is:
 *
 *     000000000000000010101000000000000000000000000000000000000000000
 *
 * Examples
 *
 *  uint64_t value = getBitField(data, 2, 4);
 *
 * Returns the value of the requested bit field.
 */
uint64_t getBitField(uint64_t data, int startPos, int numBits);

/* Public: Set the bit field in the given data array to the new value.
 *
 * data - a byte array with size at least startPos + numBits.
 * value - the value to set in the bit field.
 * startPos - the starting index of the bit field (beginning from 0).
 */
void setBitField(uint64_t* data, uint64_t value, int startPos, int numBits);

#endif // _BITFIELD_H_

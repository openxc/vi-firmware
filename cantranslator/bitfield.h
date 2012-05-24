#ifndef _BITFIELD_H_
#define _BITFIELD_H_

#include <stdint.h>

/* Public: Reads a subset of bits from a byte array.
 *
 * data - the bytes in question.
 * startPos - the starting index of the bit field (beginning from 0).
 * numBits - the width of the bit field to extract.
 *
 * Examples
 *
 *  unsigned long value = getBitField(data, 2, 4);
 *
 * Returns the value of the requested bit field.
 */
unsigned long getBitField(uint8_t* data, int startPos, int numBits);

/* Public: Set the bit field in the given data array to the new value.
 *
 * data - a byte array with size at least startPos + numBits.
 * value - the value to set in the bit field.
 * startPos - the starting index of the bit field (beginning from 0).
 */
void setBitField(uint8_t* data, unsigned long value, int startPos, int numBits);

#endif // _BITFIELD_H_

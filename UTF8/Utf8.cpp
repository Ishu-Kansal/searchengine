#include "Utf8.h"

// SizeOfUTF8 tells the number of bytes it will take to encode the
// specified value.

// SizeOfUTF8( GetUtf8( p ) ) does not tell how many bytes encode
// the character pointed to by p because p may point to a malformed
// character.

size_t SizeOfUtf8(Unicode c)
{
  uint8_t leftmostBit = 0;
  while (c)
  {
    ++leftmostBit;
    c >>= 1;
  }
  return (leftmostBit >= 0) + (leftmostBit > 7) + (leftmostBit > 11);
}

// IndicatedLength looks at the first byte of a Utf8 sequence
// and determines the expected length.  Useful for avoiding
// buffer overruns when reading Utf8 characters.  Return 1
// for an invalid first byte.

size_t IndicatedLength(const Utf8 *p)
{
  uint16_t mask = 1 << 7;
  size_t numSet = 0;
  while (*p & mask)
  {
    ++numSet;
    mask >>= 1;
  }
  if (numSet <= 1)
    return 1;
  return numSet;
}

// Get the UTF-8 character as a Unicode value.
// If it's an invalid UTF-8 encoding for a U-16
// character, return the special malformed
// character code.

Unicode GetUtf8(const Utf8 *p)
{
  const size_t length = IndicatedLength(p);
  switch (length)
  {
  case 1:
    if (p[0] >> 7)
      return ReplacementCharacter;
    return p[0];
  case 2:
    if ((p[0] & 0b00011110) == 0b0000)
      return ReplacementCharacter;
    if ((p[1] >> 6) != 0b10)
      return ReplacementCharacter;
    return ((p[0] & 0b00111111) << 6) | (p[1] & 0b00111111);
  case 3:
    if ((p[0] & 0b00001111) == 0b00 && (p[1] & 0b00100000) == 0b00)
      return ReplacementCharacter;
    if ((p[1] >> 6) != 0b10 || (p[2] >> 6) != 0b10)
      return ReplacementCharacter;
    return ((((p[0] & 0b00111111) << 6) | (p[1] & 0b00111111)) << 6) |
           (p[2] & 0b00111111);
  default:
    return ReplacementCharacter;
  }
}

// Bounded version.  bound = one past last valid
// byte.  bound == 0 means no bounds checking.
// If a character runs past the last valid byte,
// return the replacement character.

Unicode GetUtf8(const Utf8 *p, const Utf8 *bound);

// NextUtf8 will determine the length of the Utf8 character at p
// by examining the first byte, then scan forward that amount,
// stopping early if it encounters an invalid continuation byte
// or the bound, if specified.

const Utf8 *NextUtf8(const Utf8 *p, const Utf8 *bound = nullptr);

// Scan backward for the first PREVIOUS byte which could
// be the start of a UTF-8 character.

const Utf8 *PreviousUtf8(const Utf8 *p);

// Write a Unicode character in UTF-8, returning one past
// the end.

Utf8 *WriteUtf8(Utf8 *p, Unicode c);
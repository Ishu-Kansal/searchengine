#include "Utf8.h"

#include <cstdlib>
#include <iostream>

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
    return ((p[0] & 0b00011111) << 6) | (p[1] & 0b00111111);
  case 3:
    if ((p[0] & 0b00001111) == 0b0000 && (p[1] & 0b00100000) == 0b0)
      return ReplacementCharacter;
    if ((p[1] >> 6) != 0b10 || (p[2] >> 6) != 0b10)
      return ReplacementCharacter;
    return ((((p[0] & 0b00001111) << 6) | (p[1] & 0b00111111)) << 6) |
           (p[2] & 0b00111111);
  default:
    return ReplacementCharacter;
  }
}

// Bounded version.  bound = one past last valid
// byte.  bound == 0 means no bounds checking.
// If a character runs past the last valid byte,
// return the replacement character.

Unicode GetUtf8(const Utf8 *p, const Utf8 *bound)
{
  const size_t length = IndicatedLength(p);
  if (bound && p + length > bound)
    return ReplacementCharacter;
  else
    return GetUtf8(p);
}

const Utf8 *FindNext(const Utf8 *p, const Utf8 *bound, size_t length)
{
  const Utf8 *start = p + 1;
  for (size_t i = 1; i < length; ++i, ++start)
  {
    if (*start >> 6 != 0b10)
      return start;
  }
  return start;
}

// NextUtf8 will determine the length of the Utf8 character at p
// by examining the first byte, then scan forward that amount,
// stopping early if it encounters an invalid continuation byte
// or the bound, if specified.

const Utf8 *NextUtf8(const Utf8 *p, const Utf8 *bound)
{
  const size_t initLength = IndicatedLength(p);
  switch (initLength)
  {
  case 1:
    return p + 1;
  case 2:
    if ((p[0] & 0b00011110) == 0)
      return FindNext(p, bound, initLength);
    break;
  case 3:
    if ((p[0] == (0b111 << 5)) && ((p[1] >> 5) == 0b100))
      return FindNext(p, bound, initLength);
    break;
  case 4:
    if (p[0] == 0b11110000 && ((p[1] >> 4) == 0b1000))
      return FindNext(p, bound, initLength);
    break;
  case 5:
    if (p[0] == 0b11111000 && ((p[1] >> 3) == 0b10000))
      return FindNext(p, bound, initLength);
    break;
  default:
    break;
  }
  const Utf8 *curPtr = p + 1;
  for (size_t i = 1; i < initLength; ++i, ++curPtr)
  {
    if (curPtr == bound)
      return curPtr;
    if ((*curPtr >> 6) != 0b10)
      return curPtr;
  }
  return p + initLength;
}

// Scan backward for the first PREVIOUS byte which could
// be the start of a UTF-8 character.

const Utf8 *PreviousUtf8(const Utf8 *p)
{
  do
  {
    --p;
  } while ((*p >> 6) == 0b10);
  return p;
}

// Write a Unicode character in UTF-8, returning one past
// the end.

Utf8 *WriteUtf8(Utf8 *p, Unicode c)
{
  static const Utf8 block = 0b10000000;
  size_t length = SizeOfUtf8(c);
  switch (length)
  {
  case 1:
    *p = static_cast<Utf8>(c);
    return p + 1;
  case 2:
    *p = 0b11000000 | (c >> 6);
    *(p + 1) = block | (c & ((1 << 6) - 1));
    return p + 2;
  case 3:
    *p = 0b11100000 | (c >> 12);
    *(p + 1) = block | ((c >> 6) & ((1 << 6) - 1));
    *(p + 2) = block | (c & ((1 << 6) - 1));
    return p + 3;
  default:
    std::cerr << "Oops! Invalid length of UTF-8" << std::endl;
    exit(EXIT_FAILURE);
  }
}

// UTF-8 String compares.
// Same return values as strcmp( ).

int StringCompare(const Utf8 *a, const Utf8 *b)
{
  // -1 if a < b, 0 if a = b, 1 if a > b
  // https://en.cppreference.com/w/c/string/byte/strcmp
  while (*a && *b)
  {
    if (*a != *b)
      return *a - *b;
    ++a;
    ++b;
  }
  return *a - *b;
}
// Unicode string compare up to 'N' UTF-8 characters (not bytes)
// from two UTF-8 strings.

int StringCompare(const Utf8 *a, const Utf8 *b, size_t N)
{
  size_t i = 0;
  while (*a && *b && i++ < N)
  {
    if (*a++ != *b)
      return *a - *b;
    ++a;
    ++b;
  }
  return *a - *b;
}

// Only works if UTF-8 character is 1 byte (aka ascii)
Utf8 ToLower(const Utf8 *a)
{
  if (*a >= 0b01000001 && *a <= 0b01011010)
    return *a + 26;
}

// Case-independent compares.
int StringCompareI(const Utf8 *a, const Utf8 *b)
{
  const Utf8 *aStart = a;
  const Utf8 *bStart = b;
  while (*a && *b)
  {
    if (IndicatedLength(a) != 1 || IndicatedLength(b) != 1)
    {
    }
    char lower_a = ToLower(a);
    char lower_b = ToLower(b);
    if (lower_a != lower_b)
      return *a - *b;
    ++a;
    ++b;
  }
  return *a - *b;
}

int StringCompareI(const Utf8 *a, const Utf8 *b, size_t N)
{
  const Utf8 *aStart = a;
  const Utf8 *bStart = b;
  size_t i = 0;
  while (*a && *b && i++ < N)
  {
    if (IndicatedLength(a) != 1 || IndicatedLength(b) != 1)
    {
      return StringCompare(aStart, bStart);
    }
    char lower_a = ToLower(a);
    char lower_b = ToLower(b);
    if (lower_a != lower_b)
      return *a - *b;
    ++a;
    ++b;
  }
  return StringCompare(a, b, N);
}

// Lower-case only Ascii characters < 0x80 except
// spaces and control characters.

Unicode ToLower(Unicode c)
{
  if (c >= 0b01000001 && c <= 0b01011010)
    return c + 26;
}

// Identify Unicode code points representing Ascii
// punctuation, spaces and control characters.

bool IsPunctuation(Unicode c);
bool IsSpace(Unicode c);
bool IsControl(Unicode c);
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
  return (c >= 0) + (c > 0x7F) + (c > 0x7FF);
}

// IndicatedLength looks at the first byte of a Utf8 sequence
// and determines the expected length.  Useful for avoiding
// buffer overruns when reading Utf8 characters.  Return 1
// for an invalid first byte.

<<<<<<< HEAD
size_t IndicatedLength(const Utf8 *p) {
    uint16_t mask = 1 << 7;
    size_t numSet = 0;
    while (*p & mask) {
        ++numSet;
        mask >>= 1;
    }
    if (numSet == 0 || numSet > 6) return -1;
    return numSet;
=======
size_t IndicatedLength(const Utf8 *p)
{
  uint16_t mask = 1 << 7;
  size_t numSet = 0;
  while (*p & mask)
  {
    ++numSet;
    mask >>= 1;
  }
  if (numSet < 1 || numSet > 6)
    return 1;
  return numSet;
>>>>>>> f2d37be (Fixed boundary issues)
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

// NextUtf8 will determine the length of the Utf8 character at p
// by examining the first byte, then scan forward that amount,
// stopping early if it encounters an invalid continuation byte
// or the bound, if specified.

const Utf8 *NextUtf8(const Utf8 *p, const Utf8 *bound)
{
  const size_t initLength = IndicatedLength(p);
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
  static const Utf8 block = 0b10000000, block_mask = (1 << 6) - 1;
  if (c >> 11)
  {
    *p = 0b11100000 | (c >> 12);
    *(p + 1) = block | ((c >> 6) & block_mask);
    *(p + 2) = block | (c & block_mask);
    return p + 3;
  }
  else if (c >> 7)
  {
    *p = 0b11000000 | (c >> 6);
    *(p + 1) = block | (c & block_mask);
    return p + 2;
  }
  else
  {
    *p = c;
    return p + 1;
  }
}

// UTF-8 String compares.
// Same return values as strcmp( ).

int StringCompare(const Utf8 *a, const Utf8 *b)
{
  // -1 if a < b, 0 if a = b, 1 if a > b
  // https://en.cppreference.com/w/c/string/byte/strcmp
  for (; *a && *b; ++a, ++b)
  {
    if (*a < *b)
      return -1;
    if (*b < *a)
      return 1;
  }
  return *a == *b ? 0 : (*a < *b ? -1 : 1);
}
// Unicode string compare up to 'N' UTF-8 characters (not bytes)
// from two UTF-8 strings.

int StringCompare(const Utf8 *a, const Utf8 *b, size_t N)
{
  for (size_t i = 0; i < N && *a && *b; ++i)
  {
    size_t l1 = IndicatedLength(a), l2 = IndicatedLength(b);
    if (l1 < l2)
      return -1;
    if (l2 < l1)
      return 1;
    for (size_t i = 0; i < l1; ++i, ++a, ++b)
    {
      if (*a < *b)
        return -1;
      if (*b < *a)
        return 1;
    }
    a += l1;
    b += l2;
  }
  return 0;
}

// Case-independent compares.
int StringCompareI(const Utf8 *a, const Utf8 *b)
{
  for (; *a && *b; ++a, ++b)
  {
    if (IndicatedLength(a) == 1 && IndicatedLength(b) == 1)
    {
      const Unicode c1 = GetUtf8(a), c2 = GetUtf8(b);
      const Unicode lower1 = ToLower(c1), lower2 = ToLower(c2);
      if (c1 < c2)
        return -1;
      if (c2 < c1)
        return 1;
    }
    else
    {
      if (*a < *b)
        return -1;
      if (*b < *a)
        return 1;
    }
  }
  return *a == *b ? 0 : (*a < *b ? -1 : 1);
}

int StringCompareI(const Utf8 *a, const Utf8 *b, size_t N)
{
  for (size_t i = 0; i < N && *a && *b; ++i)
  {
    size_t l1 = IndicatedLength(a), l2 = IndicatedLength(b);
    if (l1 < l2)
      return -1;
    if (l2 < l1)
      return 1;
    for (size_t i = 0; i < l1; ++i, ++a, ++b)
    {
      if (*a < *b)
        return -1;
      if (*b < *a)
        return 1;
    }
    a += l1;
    b += l2;
  }
  return 0;
}

// Lower-case only Ascii characters < 0x80 except
// spaces and control characters.

Unicode ToLower(Unicode c)
{
  if ('A' <= c && c <= 'Z')
    return (c - 'A') + 'a';
  else
    return c;
}

// Identify Unicode code points representing Ascii
// punctuation, spaces and control characters.

bool IsPunctuation(Unicode c)
{
  static const char punct[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
  for (size_t i = 0; i < sizeof(punct); ++i)
  {
    if (c == punct[i])
      return true;
  }
  return false;
}
bool IsSpace(Unicode c)
{
  static const char spaces[] = " \f\n\r\t\v";
  for (size_t i = 0; i < sizeof(spaces); ++i)
  {
    if (c == spaces[i])
      return true;
  }
  return false;
}
bool IsControl(Unicode c)
{
  return (0x00 <= c && c <= 0x1f) || (c == 0x7f);
}

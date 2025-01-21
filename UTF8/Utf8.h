// Utf8.h
//
// Nicole Hamilton, nham@umich.edu

// This file defines some basic utility functions for reading and
// writing UTF-8 characters, translating to Unicode, and comparing UTF-8
// strings.


#pragma once

#include <cstddef>
#include <cstdint>

typedef uint16_t Unicode;
typedef uint8_t  Utf8;


// From http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8

//   Valid UTF-8 Sequences:
//
//   U-00000000 - U-0000007F:   0xxxxxxx 7 bits
//   U-00000080 - U-000007FF:   110xxxxx 10xxxxxx 11 bits
//   U-00000800 - U-0000FFFF:   1110xxxx 10xxxxxx 10xxxxxx 16 bits
//   U-00010000 - U-001FFFFF:   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx 21 bits
//   U-00200000 - U-03FFFFFF:   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 26 bits
//   U-04000000 - U-7FFFFFFF:   1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 31 bits
//
//   Invalid, Overlong UTF-8 Sequences which should never be accepted
//   because of the security risk:
//
//                              1100000x (10xxxxxx)
//                              11100000 100xxxxx (10xxxxxx)
//                              11110000 1000xxxx (10xxxxxx 10xxxxxx)
//                              11111000 10000xxx (10xxxxxx 10xxxxxx 10xxxxxx)

// As described in http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt,
// we will follow convention, which is to replace any malformed characters
// with the replacement character, 0xfffd.

const Unicode  ReplacementCharacter = 0xfffd;

// Utf8 and Unicode text files are often marked with a byte order
// mark.  If you try to read a file as either Utf8 or Unicode and
// you see this value, that confirms you're reading it correctly.

// If there's no BOM on a text file, it's probably ASCII but might
// be ANSI with the high half (0x80 to 0xff) internationalized.

const Unicode  ByteOrderMark = 0xfeff;

// The Utf8 byte order mark is the same 0xfeff BOM character value
// but written out as Utf8.

const Utf8     Utf8BOMString[ ] = { 0xef, 0xbb, 0xbf };

// SizeOfUTF8 tells the number of bytes it will take to encode the
// specified value.

// SizeOfUTF8( GetUtf8( p ) ) does not tell how many bytes encode
// the character pointed to by p because p may point to a malformed
// character.

size_t SizeOfUtf8( Unicode c );

// IndicatedLength looks at the first byte of a Utf8 sequence
// and determines the expected length.  Useful for avoiding
// buffer overruns when reading Utf8 characters.  Return 1
// for an invalid first byte.

size_t IndicatedLength( const Utf8 *p );

// Get the UTF-8 character as a Unicode value.
// If it's an invalid UTF-8 encoding for a U-16
// character, return the special malformed
// character code.

Unicode GetUtf8( const Utf8 *p );

// Bounded version.  bound = one past last valid
// byte.  bound == 0 means no bounds checking.
// If a character runs past the last valid byte,
// return the replacement character.

Unicode GetUtf8( const Utf8 *p, const Utf8 *bound );

// NextUtf8 will determine the length of the Utf8 character at p
// by examining the first byte, then scan forward that amount,
// stopping early if it encounters an invalid continuation byte
// or the bound, if specified.

const Utf8 *NextUtf8( const Utf8 *p, const Utf8 *bound = nullptr );


// Scan backward for the first PREVIOUS byte which could
// be the start of a UTF-8 character.

const Utf8 *PreviousUtf8( const Utf8 *p );

// Write a Unicode character in UTF-8, returning one past
// the end.

Utf8 *WriteUtf8( Utf8 *p, Unicode c );



//
// Following is not part of the homework.
//


// UTF-8 String compares.
// Same return values as strcmp( ).

int StringCompare( const Utf8 *a, const Utf8 *b );

// Unicode string compare up to 'N' UTF-8 characters (not bytes)
// from two UTF-8 strings.

int StringCompare( const Utf8 *a, const Utf8 *b, size_t N );

// Case-independent compares.

int StringCompareI( const Utf8 *a, const Utf8 *b );
int StringCompareI( const Utf8 *a, const Utf8 *b, size_t N );

// Lower-case only Ascii characters < 0x80 except
// spaces and control characters.

Unicode ToLower( Unicode c );

// Identify Unicode code points representing Ascii
// punctuation, spaces and control characters.

bool IsPunctuation( Unicode c );
bool IsSpace( Unicode c );
bool IsControl( Unicode c );

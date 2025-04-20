#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

[[nodiscard]] inline uint8_t SizeOf(size_t offset) {
  if (offset < (1ULL << 7)) {
    return 1;
  } else if (offset < (1ULL << 14)) {
    return 2;
  } else if (offset < (1ULL << 21)) {
    return 3;
  } else if (offset < (1ULL << 28)) {
    return 4;
  } else if (offset < (1ULL << 35)) {
    return 5;
  } else if (offset < (1ULL << 42)) {
    return 6;
  } else if (offset < (1ULL << 49)) {
    return 7;
  } else if (offset < (1ULL << 56)) {
    return 8;
  } else if (offset < (1ULL << 63)) {
    return 9;
  } else {
    return 10;
  }
}

inline uint8_t *encodeVarint(uint64_t val, uint8_t *buf) {
  while (val >= 128) {
    *buf++ = 0x80 | (val & 0x7F);
    val >>= 7;
  }
  *buf++ = uint8_t(val);
  return buf;
}

/*
inline const uint8_t *decodeVarint(const uint8_t *buf, uint64_t &val) {
  val = 0;
  unsigned shift = 0;
  for (int i = 0; i < 1000; ++i) {
    uint8_t byte = *buf++;
    val |= (uint64_t(byte & 0x7F) << shift);
    if ((byte & 0x80) == 0) return buf;
    shift += 7;
  }
  assert(false);
}
*/

inline const uint8_t *decodeVarint(const uint8_t *buf, uint64_t &val) 
{
    uint8_t byte;
    val = 0;

    do {
        // Byte 1 (Shift 0)
        byte = *buf++;
    
        val = (uint64_t(byte & 0x7F));
        if ((byte & 0x80) == 0) break; 

        // Byte 2 (Shift 7)
        byte = *buf++;
        val |= (uint64_t(byte & 0x7F) << 7);
        if ((byte & 0x80) == 0) break;

        // Byte 3 (Shift 14)
        byte = *buf++;
        val |= (uint64_t(byte & 0x7F) << 14);
        if ((byte & 0x80) == 0) break; 

        // Byte 4 (Shift 21)
        byte = *buf++;
        val |= (uint64_t(byte & 0x7F) << 21);
        if ((byte & 0x80) == 0) break; 

        // Byte 5 (Shift 28)
        byte = *buf++;
        val |= (uint64_t(byte & 0x7F) << 28);
        if ((byte & 0x80) == 0) break;

        // Byte 6 (Shift 35)
        byte = *buf++;
        val |= (uint64_t(byte & 0x7F) << 35);
        if ((byte & 0x80) == 0) break;

        // Byte 7 (Shift 42)
        byte = *buf++;
        val |= (uint64_t(byte & 0x7F) << 42);
        if ((byte & 0x80) == 0) break; 

        // Byte 8 (Shift 49)
        byte = *buf++;
        val |= (uint64_t(byte & 0x7F) << 49);
        if ((byte & 0x80) == 0) break; 

        // Byte 9 (Shift 56)
        byte = *buf++;
        val |= (uint64_t(byte & 0x7F) << 56);
        if ((byte & 0x80) == 0) break; 

        byte = *buf++;
        val |= (uint64_t(byte & 0x7F) << 63); 
        if ((byte & 0x80) == 0) break;

    } while (false);
    return buf;
}
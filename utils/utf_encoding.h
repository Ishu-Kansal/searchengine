#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

[[nodiscard]] inline size_t SizeOf(size_t offset) {
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
  *buf = uint8_t(val);
  return buf;
}

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

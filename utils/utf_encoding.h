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

struct GroupVarintDecodeInfo 
 {
   uint8_t lengths[4];      // Byte length (1-4) for each of the 4 integers
   uint8_t totalDataBytes;  // Total bytes used by the 4 integers
 };
 
 
 inline void precomputeGroupVarintDecodeTable(GroupVarintDecodeInfo g_groupVarintDecodeTable[256], bool g_groupVarintTableInitialized) 
 {
   if (g_groupVarintTableInitialized) return;
 
   for (int tag = 0; tag < 256; ++tag) 
   {
       uint8_t totalBytes = 0;
       for (int i = 0; i < 4; ++i) 
       {
         uint8_t length_code = (tag >> (6 - 2 * i)) & 0b11;
         uint8_t length = length_code + 1; 
         g_groupVarintDecodeTable[tag].lengths[i] = length;
         totalBytes += length;
       }
       g_groupVarintDecodeTable[tag].totalDataBytes = totalBytes;
   }
   g_groupVarintTableInitialized = true;
 }
 
 inline int getBytesNeeded(uint32_t val) 
 {
   if (val < (1 << 8)) return 1;
   if (val < (1 << 16)) return 2;
   if (val < (1 << 24)) return 3;
   return 4;
 }
 
 inline uint8_t *encodeGroupVarint(const uint32_t *values, uint8_t *buf) {
   uint8_t tag_byte = 0;
   uint8_t *data_ptr = buf + 1;
 
   for (int i = 0; i < 4; ++i) {
       uint32_t val = values[i];
       int num_bytes = getBytesNeeded(val);
       assert(num_bytes >= 1 && num_bytes <= 4);
       uint8_t tag_bits = num_bytes - 1;
       tag_byte |= (tag_bits << (6 - 2 * i));
       for (int j = 0; j < num_bytes; ++j) {
           *data_ptr++ = (val >> (j * 8)) & 0xFF;
       }
   }
   *buf = tag_byte;
   return data_ptr;
 }
 
 inline const uint8_t* decodeGroupVarint(
   const uint8_t* dataPtr,
   uint32_t* outputBuffer,
   GroupVarintDecodeInfo g_groupVarintDecodeTable[256], 
   bool g_groupVarintTableInitialized
 )
 {
   if (!g_groupVarintTableInitialized) 
   {
       precomputeGroupVarintDecodeTable(g_groupVarintDecodeTable, g_groupVarintTableInitialized);
   }
   // First byte is tag/header, grab it and advance pointer
   uint8_t tag = *dataPtr++;
   // Use precomputed table to get lens
   const GroupVarintDecodeInfo& decodeInfo = g_groupVarintDecodeTable[tag];
 
   for (int i = 0; i < 4; ++i) 
   {
       uint8_t num_bytes = decodeInfo.lengths[i];
 
       uint32_t val = 0;
       switch (num_bytes) 
       {
           case 1:
             val = *dataPtr;
             break;
           case 2:
             val = *dataPtr | (uint32_t(*(dataPtr+1)) << 8);
             break;
           case 3:
             val = *dataPtr | (uint32_t(*(dataPtr+1)) << 8) | (uint32_t(*(dataPtr+2)) << 16);
             break;
           case 4:
             memcpy(&val, dataPtr, 4);
             break;
           default:
             exit(EXIT_FAILURE);
       }
       outputBuffer[i] = val; 
       dataPtr += num_bytes; 
     } 
   return dataPtr; 
 }
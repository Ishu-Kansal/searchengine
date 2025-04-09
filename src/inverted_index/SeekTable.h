#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "../../utils/utf_encoding.h"

using PostOffset = uint64_t;
using PostPosition = uint64_t;

class SeekTable;
struct SeekEntry {
  SeekEntry(PostOffset postOffset, PostPosition location)
      : postOffset(postOffset), location(location) {}

  PostOffset postOffset = -1;
  PostPosition location = -1;
};
struct TableDecode {
  const uint8_t *buf;
  SeekTable *table;
};

class SeekTable {
 public:
  const static size_t OFFSET = 13;         // new entry every 8192 words
  const static size_t LOG_TABLE_SIZE = 7;  // allows 2^7 = 128 seek entries

  // total: can store up to 2^20 = 1M posts

  const static size_t MASK = (1 << LOG_TABLE_SIZE) - 1;
  SeekTable() = default;

  void addEntry(uint64_t postOffset, uint64_t location) {
    const uint64_t mask = (location >> OFFSET) & MASK;
    while (seek_table.size() <= mask)
      seek_table.emplace_back(postOffset, location);
  }

  size_t size() const { return seek_table.size(); }

  size_t header_size() const { return SizeOf(seek_table.size()); }

  size_t data_size() const {
    size_t ans = 0;
    for (const auto &entry : seek_table) {
      ans += SizeOf(entry.location);
      ans += SizeOf(entry.postOffset);
    }
    return ans;
  }

  static uint8_t *encode_header(uint8_t *buf, const SeekTable &table) {
    return encodeVarint(table.size(), buf);
  }

  static uint8_t *encode_data(uint8_t *buf, const SeekTable &table) {
    for (const auto &entry : table.seek_table) {
      buf = encodeVarint(entry.postOffset, buf);
      buf = encodeVarint(entry.location, buf);
    }
    return buf;
  }

  static uint8_t *encode_table(uint8_t *buf, const SeekTable &table) {
    buf = encode_header(buf, table);
    return encode_data(buf, table);
  }

  static TableDecode decode_table(const uint8_t *buf) {
    SeekTable *table = new SeekTable{};
    uint64_t size, pos, loc;
    buf = decodeVarint(buf, size);
    for (size_t i = 0; i < size; ++i) {
      buf = decodeVarint(buf, pos);
      buf = decodeVarint(buf, loc);
      table->seek_table.emplace_back(pos, loc);
    }
    return {buf, table};
  }

 private:
  std::vector<SeekEntry> seek_table{};
};

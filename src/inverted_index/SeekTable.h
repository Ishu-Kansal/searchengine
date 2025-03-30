#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "../../utils/utf_encoding.h"

using PostOffset = uint64_t;
using PostPosition = uint64_t;

struct SeekEntry {
  SeekEntry(PostOffset postOffset, PostPosition location)
      : postOffset(postOffset), location(location) {}

  PostOffset postOffset = -1;
  PostPosition location = -1;
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

  size_t header_size() const { return sizeof(size_t); }

  size_t data_size() const { return sizeof(SeekEntry) * seek_table.size(); }

  static uint8_t *encode_header(uint8_t *buf, const SeekTable &table) {
    return encodeVarint(table.size(), buf, SizeOf(table.size()));
  }

  static uint8_t *encode_data(uint8_t *buf, const SeekTable &table) {
    for (const auto &entry : table.seek_table) {
      buf = encodeVarint(entry.postOffset, buf, SizeOf(entry.postOffset));
      buf = encodeVarint(entry.location, buf, SizeOf(entry.location));
    }
    return buf;
  }

  static uint8_t *encode_table(uint8_t *buf, const SeekTable &table) {
    buf = encode_header(buf, table);
    return encode_data(buf, table);
  }

 private:
  std::vector<SeekEntry> seek_table{};
};

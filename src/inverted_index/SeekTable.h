#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

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

  size_t header_size() const { return sizeof(size_t); }

  size_t data_size() const { return sizeof(SeekEntry) * seek_table.size(); }

 private:
  std::vector<SeekEntry> seek_table{};
};

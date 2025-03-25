#pragma once

#include <cstdint>
#include <vector>

#include "IndexBlob.h"

struct SeekEntry {
  SeekEntry(PostOffset postOffset, PostPosition location)
      : postOffset(postOffset), location(location) {}

  PostOffset postOffset = -1;
  PostPosition location = -1;
};

const static SeekEntry EMPTY = SeekEntry(-1, -1);

class SeekTable {
 public:
  const static size_t OFFSET = 13;         // new entry every 8192 words
  const static size_t LOG_TABLE_SIZE = 7;  // allows 2^7 = 128 seek entries

  // total: can store up to 2^20 = 1M posts

  const static size_t MASK = (1 << LOG_TABLE_SIZE) - 1;

  static SeekTable *createSeekTable(const PostingListBlob &pl) {
    SeekTable *table = new SeekTable(pl);
  }

  SeekTable(const PostingListBlob &pl) {
    seek_table.resize(1 << LOG_TABLE_SIZE);
    uint64_t pos = 0;

    const char *start =
        reinterpret_cast<const char *>(&pl) + sizeof(pl.blobHeader);
    const char *end =
        reinterpret_cast<const char *>(&pl) + pl.blobHeader.BlobSize;

    // Linear search
    while (start < end) {
      const SerialPost *post = reinterpret_cast<const SerialPost *>(start);
      uint64_t delta = 0;
      decodeVarint(post->delta, delta);
      pos += delta;
      size_t mask = (pos >> OFFSET) & MASK;
      size_t TEMP_LOC = -1;  // ???
      if (seek_table[mask].location == -1) {
        seek_table[mask] = SeekEntry(pos, TEMP_LOC);
      }
      start += post->totalLen;
    }
  }

  const SeekEntry &FindStart(size_t seek) const {
    const size_t index = (seek >> OFFSET) & MASK;
    return seek_table[index];
  }

 private:
  std::vector<SeekEntry> seek_table;
};

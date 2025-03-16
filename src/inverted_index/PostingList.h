#include <vector>

#include "utils/cunique_ptr.h"
#include "utils/unrolled_linked_list.h"

class PostingList {
 public:
 private:
  // Common Header

  uint64_t index_freq;     // Number of occurrences of this token in the index
  uint64_t document_freq;  // Number of documents in which this token occurs.
  uint64_t size;           // Size of the list for skipping over collisions.
  char type;  // Type of token: end-of-document, word in anchor, URL, title or
              // body.
  // Seek List
  // Type specific data?
  UnrolledLinkList<Post> posting_list;
};

using delta_t = unsigned char;

size_t IndicatedLength(const delta_t del[]) {
  uint16_t mask = 1 << 7;
  size_t numSet = 0;
  while (*del & mask) {
    ++numSet;
    mask >>= 1;
  }
  if (numSet < 1 || numSet > 6) return 1;
  return numSet;
}

size_t SizeOfDelta(int offset) {
  return (offset < 0x80) + (offset < 0x800) + (offset < 0x10000) +
         (offset < 0x200000) + (offset < 0x4000000);
}

int delta_to_int(const delta_t del[]) {
  static const delta_t block_mask = (1 << 6) - 1;
  const size_t num_bytes = IndicatedLength(del);
  if (num_bytes == 1) return del[0];
  int cur = [&](const delta_t first) {
    return first & ((1 << (7 - num_bytes)) - 1);
  }(del[0]);
  for (int i = 1; i < num_bytes; ++i) {
    cur <<= 6;
    cur |= (del[i] & block_mask);
  }
  return cur;
}

delta_t *int_to_delta(int offset) {
  static const delta_t block_mask = (1 << 6) - 1;
  const size_t num_bytes = SizeOfDelta(offset);
  switch (num_bytes) {
    case 1:
      return new delta_t(offset);
    default:
      delta_t *del = new delta_t[num_bytes];
      for (int cur = num_bytes - 1; cur >= 0; --cur) {
        del[cur] = num_bytes & block_mask;
      }
      del[0] |= ((1 << num_bytes) - 1) << (8 - num_bytes);
      return del;
  }
}

struct Post {
  // uint32_t delta;
  cunique_ptr<delta_t[]> delta{};
  // Type specific data?
};
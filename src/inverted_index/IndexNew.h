#pragma once

#include <deque>
#include <string>
#include <vector>

#include "../../HashTable/HashTableStarterFiles/HashBlob.h"
#include "../../HashTable/HashTableStarterFiles/HashTable.h"
#include "../../utils/cstring_view.h"
#include "../../utils/cunique_ptr.h"
#include "../../utils/unrolled_linked_list.h"
#include "SeekTable.h"

constexpr unsigned char TITLE_FLAG = 0x01;
constexpr char TITLE_MARKER = '!';

// unexpected to come up, can use as posting_list for end_doc data
// matches zero docs on google
constexpr const char *end_doc_hash = "!#$%!#13513sfas";

[[nodiscard]] inline size_t SizeOfDelta(size_t offset) {
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

inline uint8_t *encodeVarint(uint64_t val, uint8_t *buf, size_t num_bytes) {
  for (size_t i = 0; i < num_bytes - 1; ++i) {
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

struct Post {
  Post(uint64_t loc) : location{loc} {}

  static uint8_t *Write(uint8_t *buf, Post p, uint64_t prev) {
    const uint64_t offset = p.location - prev;
    const uint64_t numRequired = SizeOfDelta(offset);
    encodeVarint(offset, buf, numRequired);
    return buf + numRequired;
  }

  uint64_t location{};
};

struct EndDocData {
  EndDocData(uint64_t l1, uint64_t l2, uint32_t d, uint16_t u, uint16_t t)
      : location{l1},
        list_index{l2},
        doc_length{d},
        url_length{u},
        title_length{t} {}

  uint64_t location{};
  uint64_t list_index{};
  uint32_t doc_length{};
  uint16_t url_length{};
  uint16_t title_length{};
};

struct Doc {
  std::string url{};
  size_t staticRank{};

  Doc(const std::string &url_, size_t staticRank_)
      : url(url_), staticRank(staticRank_) {}
  Doc(std::string &&url_, size_t staticRank_)
      : url{std::move(url_)}, staticRank{staticRank_} {}
  Doc(const char *c1, const char *c2, size_t staticRank_)
      : url(c1, c2), staticRank(staticRank_) {}
};

class PostingList {
 public:
  void add_post(size_t pos) {
    size_t delta = pos - prev_pos;
    size_t n = SizeOfDelta(delta);
    uint8_t buf[n];
    encodeVarint(delta, buf, n);
    for (size_t i = 0; i < n; ++i) posting_list.push_back(buf[i]);
  }

  [[nodiscard]] size_t header_size() const {
    return sizeof(size_t) + table.header_size();
  }

  [[nodiscard]] size_t data_size() const {
    return posting_list.size() + table.data_size();
  }

  [[nodiscard]] size_t total_size() const {
    return header_size() + data_size();
  }

  auto begin() { return posting_list.begin(); }

  auto end() { return posting_list.end(); }

  auto begin() const { return posting_list.begin(); }

  auto end() const { return posting_list.end(); }

  [[nodiscard]] size_t size() const { return posting_list.size(); }

  static uint8_t *Write(uint8_t *buf, const PostingList &p) {
    const size_t s = p.posting_list.size();
    memcpy(buf, &s, sizeof(size_t));
    buf += sizeof(size_t);
    for (const auto &post : p.posting_list) {
      *buf++ = post;
    }
    return buf;
  }

 private:
  std::deque<uint8_t> posting_list{};
  size_t prev_pos = 0;
  SeekTable table{};
};

struct EndOfDocList {
  template <class... Args>
  void add_enddoc(Args &&...args) {
    enddoc_list.emplace_back(std::forward<Args>(args)...);
  }

  auto begin() { return enddoc_list.begin(); }

  auto end() { return enddoc_list.end(); }

  auto begin() const { return enddoc_list.begin(); }

  auto end() const { return enddoc_list.end(); }

  [[nodiscard]] size_t size() const { return enddoc_list.size(); }

 private:
  std::deque<EndDocData> enddoc_list{};
};

class InvertedIndex {
 public:
  template <class... Args>
  void add_enddoc(size_t pos) {
    static std::string endDoc = "!#$%!#13513sfas";
    add_word(endDoc, pos, false);
  }

  void add_word(std::string &word, size_t pos, bool in_title) {
    // Check if the word exists in the dictionary
    if (in_title) word.push_back(TITLE_MARKER);
    auto *tuple = dictionary.Find(word.data());
    // Word doesn't exist yet
    if (!tuple) {
      // create a new posting list
      lists_of_posting_lists.emplace_back();
      PostingList &list = lists_of_posting_lists.back();

      // Add the post to the posting list
      list.add_post(pos);

      dictionary.Find(word.data(), lists_of_posting_lists.size() - 1);
    } else {
      // Word exists
      size_t index = tuple->value;

      // Update the posting list
      PostingList &list = lists_of_posting_lists[index];
      list.add_post(pos);
    }
  }

 private:
  friend class IndexChunk;

  HashTable<const char *, size_t> dictionary{};
  std::deque<PostingList> lists_of_posting_lists{};
  EndOfDocList end_of_doc_list{};
};

class IndexChunk {
 public:
  void add_url(std::string &url, size_t staticRank) {
    url_list.emplace_back(std::move(url), staticRank);
  }

  void add_enddoc() {
    inverted_word_index.add_enddoc(location);
    ++location;
  }

  void add_word(std::string &word, bool in_title = false) {
    inverted_word_index.add_word(word, location, in_title);
    ++location;
  }

  static void Write(const IndexChunk &c, size_t i) {
    std::string hashname =
        "./chunks/chunk" + std::to_string(i) + "_hashtable.bin";
    HashFile(hashname.data(), &c.inverted_word_index.dictionary);
  }

 private:
  InvertedIndex inverted_word_index{};
  std::deque<Doc> url_list{};
  uint64_t location{};
};

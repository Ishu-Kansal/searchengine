#pragma once
#include <deque>
#include <vector>

#include "../../HashTable/HashTableStarterFiles/HashTable.h"
#include "../../utils/cstring_view.h"
#include "../../utils/cunique_ptr.h"
// #include "../../utils/unrolled_linked_list.h"
#include "SeekTable.h"

constexpr unsigned char TITLE_FLAG = 0x01;
constexpr unsigned char BOLD_FLAG = 0x02;
constexpr char TITLE_MARKER = '!';

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
  uint64_t location{};
  Post() = default;
  Post(uint64_t pos) : location{pos} {}
};

struct Doc {
  std::string url;
  size_t staticRank;

  Doc(std::string &url_, size_t staticRank_)
      : url(std::move(url_)), staticRank(staticRank_) {}
  Doc(const char *c1, const char *c2, size_t staticRank_)
      : url(c1, c2), staticRank(staticRank_) {}
};

class PostingList {
 public:
  void add_post(size_t pos) {
    table.addEntry(posting_list.size(), pos);
    posting_list.emplace_back(Post(pos));
  }

  [[nodiscard]] size_t header_size() const { return sizeof(size_t); }

  auto begin() { return posting_list.begin(); }

  auto end() { return posting_list.end(); }

  auto begin() const { return posting_list.begin(); }

  auto end() const { return posting_list.end(); }

  [[nodiscard]] size_t size() const { return posting_list.size(); }

 private:
  // Linked list of posts
  std::deque<Post> posting_list;
  SeekTable table;
};

class InvertedIndex {
 public:
  void add_enddoc(size_t pos) {
    static std::string endDoc = "!#$%!#13513sfas";
    add_word(endDoc, pos, false);
  }

  void add_word(std::string &word, size_t pos, bool title = true) {
    if (title) word.push_back('!');
    // Check if the word exists in the dictionary
    auto *tuple = dictionary.Find(word);
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
  HashTable<const std::string, size_t> dictionary;
  std::vector<PostingList> lists_of_posting_lists;
};

class IndexChunk {
 public:
  void add_url(std::string &url, size_t staticRank) {
    url_list.emplace_back(url, staticRank);
  }
  void add_enddoc() {
    inverted_word_index.add_enddoc(pos);
    ++pos;
  }

  void add_word(std::string &word, bool in_title = false) {
    inverted_word_index.add_word(word, pos, in_title);
    ++pos;
  }

 private:
  std::vector<Doc> url_list;
  InvertedIndex inverted_word_index;
  uint64_t pos;
};

#pragma once
#include <deque>
#include <vector>

#include "../../HashTable/HashTableStarterFiles/HashTable.h"
#include "../../utils/cstring_view.h"
#include "../../utils/cunique_ptr.h"
// #include "../../utils/unrolled_linked_list.h"
#include "../../utils/utf_encoding.h"
#include "SeekTable.h"

constexpr unsigned char TITLE_FLAG = 0x01;
constexpr unsigned char BOLD_FLAG = 0x02;
constexpr char TITLE_MARKER = '!';

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
  friend uint8_t *encode_posting_list(uint8_t *, const PostingList &);

  void add_post(size_t pos) {
    table.addEntry(bytes, pos);
    posting_list.emplace_back(pos);
    bytes += SizeOf(pos - prev);
    prev = pos;
  }

  auto begin() { return posting_list.begin(); }

  auto end() { return posting_list.end(); }

  auto begin() const { return posting_list.begin(); }

  auto end() const { return posting_list.end(); }

  [[nodiscard]] size_t size() const { return posting_list.size(); }

 private:
  // Linked list of posts
  std::deque<Post> posting_list{};
  uint64_t bytes{};
  uint64_t prev{};
  SeekTable table{};
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

uint8_t *encode_url_list(uint8_t *buf, const std::vector<Doc> &url_list) {
  buf = encodeVarint(url_list.size(), buf);
  for (const auto &url : url_list) {
    buf = encodeVarint(url.staticRank, buf);
    buf = encodeVarint(url.url.size(), buf);
    buf = static_cast<uint8_t *>(memcpy(buf, url.url.data(), url.url.size()));
  }
  return buf;
}

size_t doc_list_required_size(const std::vector<Doc> &doc_list) {
  size_t ans = SizeOf(doc_list.size());
  for (const auto &url : doc_list) {
    ans += SizeOf(url.staticRank);
    ans += SizeOf(url.url.size());
  }
  return ans;
}

uint8_t *encode_posting_list(uint8_t *buf, const PostingList &pl) {
  // save byte start for header
  // isr will load seek table into memory; seek table needs byte offsets
  buf = SeekTable::encode_header(buf, pl.table);
  buf = SeekTable::encode_data(buf, pl.table);
  buf = encodeVarint(pl.posting_list.size(), buf);
  uint64_t prev = 0;
  for (const auto entry : pl.posting_list) {
    const uint64_t delta = entry.location - prev;
    buf = encodeVarint(delta, buf);
    prev = entry.location;
  }
  return buf;
}

void *encodeIndex(const IndexChunk &h) {}

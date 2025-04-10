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
  uint8_t staticRank;
  Doc() = default;
  Doc(std::string &url_, uint8_t staticRank_)
      : url(std::move(url_)), staticRank(staticRank_) {}
  Doc(const char *c1, const char *c2, uint8_t staticRank_)
      : url(c1, c2), staticRank(staticRank_) {}
};

class PostingList {
 public:
  friend uint8_t *encode_posting_list(uint8_t *, const PostingList &);

  void add_post(size_t pos) {
    posting_list.emplace_back(pos);
  }
  void add_word(std::string & word_)
  {
    word = (word_);
  }
  auto begin() { return posting_list.begin(); }

  auto end() { return posting_list.end(); }

  auto begin() const { return posting_list.begin(); }

  auto end() const { return posting_list.end(); }

  [[nodiscard]] size_t size() const { return posting_list.size(); }

  [[nodiscard]] const std::string & get_word() const {
    return word;
  }

 private:
  // Linked list of posts
  std::deque<Post> posting_list{};
  std::string word;
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
      list.add_word(word);
      dictionary.Find(word.data(), lists_of_posting_lists.size() - 1);
    } else {
      // Word exists
      size_t index = tuple->value;

      // Update the posting list
      PostingList &list = lists_of_posting_lists[index];
      list.add_post(pos);
    }
  }

  [[nodiscard]] const std::vector<PostingList> & get_posting_lists() const {
    return lists_of_posting_lists;
  }
  [[nodiscard]] HashTable<const std::string, size_t> & get_dictionary() {
    return dictionary;
  }
  private:
  HashTable<const std::string, size_t> dictionary;
  std::vector<PostingList> lists_of_posting_lists;
};

class IndexChunk {
 public:
  IndexChunk() : pos(0), url_list_size(0) 
  {
    url_list.reserve(100000);
  }
  void add_url(std::string &url, size_t staticRank) {
    url_list_size += url.size() + 2; // 1 byte for static rank and 1 byte for url length
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
  [[nodiscard]] const std::vector<Doc>& get_urls() const {
    return url_list;
  }
  [[nodiscard]] const std::vector<PostingList> & get_posting_lists() const {
    return inverted_word_index.get_posting_lists();
  }
  [[nodiscard]] HashTable<const std::string, size_t> & get_dictionary() {
    return inverted_word_index.get_dictionary();
  }
  [[nodiscard]] uint64_t get_url_list_size_bytes() const {
    return url_list_size;
  }
 private:
  std::vector<Doc> url_list;
  InvertedIndex inverted_word_index;
  uint64_t url_list_size;
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

/*
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
*/


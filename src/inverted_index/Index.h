#include<vector>

#include "utils/unrolled_linked_list.h"
#include "utils/cunique_ptr.h"
#include "HashTable/HashTableStarterFiles/HashTable.h"

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
    if (offset < 0) {
        return 0;
    } else if (offset < 0x80U) {
        return 1;
    } else if (offset < 0x800) {
        return 2;
    } else if (offset < 0x10000) {
        return 3;
    } else if (offset < 0x200000) {
        return 4;
    } else if (offset < 0x4000000) {
        return 5;
    } else {
        return 0;
    }
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

inline void encodeVarint(uint64_t val, uint8_t* buf, size_t num_bytes) {
    uint8_t* p = buf;

    for (size_t i = 0; i < num_bytes - 1; ++i) {
        *p++ = 0x80 | (val & 0x7F);
        val >>= 7;
    }
    *p = uint8_t(val);

}

inline void decodeVarint(const uint8_t* buf, uint64_t& val) {
    uint64_t result = 0;
    unsigned shift = 0;
    const uint8_t* p = buf;
    while (true) {
        uint8_t byte = *p++;
        result |= (uint64_t(byte & 0x7F) << shift);
        if ((byte & 0x80) == 0)
            break;
        shift += 7;
    }
    val = result;
}   


struct Post
{
    cunique_ptr<delta_t[]> delta{};
    bool title = false;
    bool bold = false;
    
    Post() = default;
    Post(cunique_ptr<delta_t[]> d, bool t, bool b) : delta(std::move(d)), title(t), bold(b) {}
    explicit Post(delta_t* d, bool t, bool b) : delta(d), title(t), bold(b) {}
};

class IndexChunk {

    public:
        void add_url(std::string url)
        {
            url_list.push_back(std::move(url)); 
        }

        void add_word(
            std::string word,
            uint64_t pos, 
            bool in_title = false, 
            bool in_bold = false, 
            bool new_document = true)
        {
            size_t delta = pos - prev_pos;
            size_t num_bytes = SizeOfDelta(delta);
            cunique_ptr<uint8_t[]> buf(new uint8_t[num_bytes]);
            encodeVarint(delta, buf.get(), num_bytes);
            inverted_word_index.add_word(word, Post(buf, in_title, in_bold), new_document);
            prev_pos = pos;
        }
    private:
        std::vector<std::string> url_list;
        InvertedIndex inverted_word_index;
        uint64_t prev_pos;

};

class InvertedIndex {

    public:

        void add_word(const std::string& word, const Post& post, bool new_document = true) 
        {
            // Check if the word exists in the dictionary
            auto* tuple = dictionary.Find(word);
            // Word doesn't exist yet
            if (!tuple) {
                // create a new posting list
                lists_of_posting_lists.emplace_back();
                PostingList& list = lists_of_posting_lists.back();
                
                // Add the post to the posting list
                list.add_post(post);
                list.increment_document_freq();
                
                dictionary.Find(word, lists_of_posting_lists.size() - 1);
            } else {
                // Word exists
                size_t index = tuple->value;
                
                // Update the posting list
                PostingList& list = lists_of_posting_lists[index];
                list.add_post(post);
                
                // Increment document frequency if this is a new document for this word
                if (new_document) {
                    list.increment_document_freq();
                }
            }
        }
    private:
        HashTable<std::string, size_t> dictionary;
        std::vector<PostingList> lists_of_posting_lists;

};

class PostingList {

    public:

        void add_post(const Post & post) {
            posting_list.push_back(std::move(Post(post)));
            index_freq++;
        }

        void increment_document_freq() {
            document_freq++;
        }
    private:

        // Common Header
        char type;              // Type: end-of-doc, word in anchor, URL, title, body.
        uint64_t index_freq;    // Number of occurrences of this token in the index
        uint64_t document_freq; // Number of documents in which this token occurs.
        uint64_t size;          // Size of the list for skipping over collisions.

        // Type Specific Data
        uint64_t doc_length;   
        uint64_t url_length;
        uint64_t title_length;
        uint64_t anchor_text_amount;
        uint64_t unique_anchor_words;

        // Linked list of posts
        UnrolledLinkList<Post> posting_list;

};

#include<vector>

#include "../../utils/unrolled_linked_list.h"
#include "../../utils/cunique_ptr.h"
#include "../../HashTable/HashTableStarterFiles/HashTable.h"

constexpr unsigned char TITLE_FLAG = 0x01;
constexpr unsigned char BOLD_FLAG  = 0x02;

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
    unsigned char flags = 0; 
    uint8_t numBytes = 0;
    cunique_ptr<uint8_t[]> delta{};
    

    Post() = default;
    Post(cunique_ptr<uint8_t[]> d, uint8_t numBytes, bool t, bool b)
      : delta(std::move(d)), numBytes(numBytes)
    {
        if (t) flags |= TITLE_FLAG;
        if (b) flags |= BOLD_FLAG;
    }
};

struct EndDocData 
    {
        uint64_t doc_length;   
        uint64_t url_length;
        uint64_t title_length;
        uint64_t anchor_text_amount;
        uint64_t unique_anchor_words;
    };
class IndexChunk {

    public:
        void add_url(std::string url)
        {
            url_list.push_back(std::move(url)); 
        }
        void add_enddoc(
            uint64_t doc_length,
            uint64_t url_length,
            uint64_t title_length,
            uint64_t anchor_text_amount,
            uint64_t unique_anchor_words
        )
        {
          EndDocData data{doc_length, url_length, title_length, anchor_text_amount, unique_anchor_words};
          inverted_word_index.add_enddoc(data);
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
            inverted_word_index.add_word(word, Post(buf, num_bytes, in_title, in_bold), new_document);
            prev_pos = pos;
        }
    private:
        std::vector<std::string> url_list;
        InvertedIndex inverted_word_index;
        uint64_t prev_pos;

};

class InvertedIndex {

    public:
        void add_enddoc(const EndDocData &endDoc) {
          end_of_doc_list.add_enddoc(endDoc);
        }
        void add_word(const std::string& word, const Post& post, bool new_document = true) 
        {
            // Check if the word exists in the dictionary
            auto* tuple = dictionary.Find(word);
            // Word doesn't exist yet
            if (!tuple) [[unlikely]] {
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
        EndOfDocList end_of_doc_list;

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

        [[nodiscard]] size_t header_size() const {
            return sizeof(type) + sizeof (index_freq) + sizeof (document_freq) + sizeof(size);  
        }

        auto begin() { return posting_list.begin(); }

        auto end() { return posting_list.end(); }

        auto begin() const { return posting_list.begin(); }

        auto end() const { return posting_list.end(); }

        [[nodiscard]] size_t size() const { return posting_list.size(); }
    private:

        // Common Header
        char type;              // Type: end-of-doc, word in anchor, URL, title, body.
        uint64_t index_freq;    // Number of occurrences of this token in the index
        uint64_t document_freq; // Number of documents in which this token occurs.
        uint64_t size;          // Size of the list for skipping over collisions. (Don't think we need)

        // Linked list of posts
        UnrolledLinkList<Post> posting_list;

};

class EndOfDocList {

public:

    void add_enddoc(const EndDocData &endDoc) 
    {
        enddoc_list.push_back(std::move(EndDocData(endDoc)));
    }

    auto begin() { return enddoc_list.begin(); }

    auto end() { return enddoc_list.end(); }

    auto begin() const { return enddoc_list.begin(); }

    auto end() const { return enddoc_list.end(); }

    [[nodiscard]] size_t size() const { return enddoc_list.size(); }
    
private:
    UnrolledLinkList<EndDocData> enddoc_list;
};
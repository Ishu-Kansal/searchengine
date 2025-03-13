#include<vector>

#include "utils/unrolled_linked_list.h"
#include "HashTable/HashTableStarterFiles/HashTable.h"

struct Post
{
    uint32_t delta;
    bool title;
    bool bold;
};

class IndexChunk {

    public:
        void add_url(std::string url)
        {
            url_list.push_back(url);
        }
        void add_word(std::string word)
        {
            return;
        }
    private:
        std::vector<std::string> url_list;
        InvertedIndex inverted_word_index;

};

class InvertedIndex {

    public:
        
    private:
        HashTable<std::string, UnrolledLinkList<Post>::Node*> dictionary;
        std::vector<PostingList> lists_of_posting_lists;

};;

class PostingList {

    public:

    private:

        // Common Header

        uint64_t index_freq;    // Number of occurrences of this token in the index
        uint64_t document_freq; // Number of documents in which this token occurs.
        uint64_t size;          // Size of the list for skipping over collisions.
        char type;              // Type: end-of-doc, word in anchor, URL, title, body.

        // Type Specific Data

        uint64_t doc_length;
        uint64_t url_length;
        uint64_t title_length;
        uint64_t anchor_text_amount;
        uint64_t unique_anchor_words;

        UnrolledLinkList<Post> posting_list;

};

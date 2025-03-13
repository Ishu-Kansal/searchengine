#include<vector>

#include "utils/unrolled_linked_list.h"
#include "HashTable/HashTableStarterFiles/HashTable.h"

struct Post
{
    uint32_t pos;
    bool title;
    bool bold;
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
            Post new_post;
            new_post.pos = pos;
            new_post.title = in_title;
            new_post.bold = in_bold;
            
            inverted_word_index.add_word(word, new_post, new_document);
        }
    private:
        std::vector<std::string> url_list;
        InvertedIndex inverted_word_index;

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

        void add_post(const Post& post) {
            posting_list.push_back(post);
            index_freq++;
        }

        void increment_document_freq() {
            document_freq++;
        }
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

        // Linked list of posts
        UnrolledLinkList<Post> posting_list;

};

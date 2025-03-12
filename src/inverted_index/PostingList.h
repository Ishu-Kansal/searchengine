#include<vector>
#include "utils/unrolled_linked_list.h"
class PostingList {

    public:

    private:
        // Common Header

        uint64_t index_freq; // Number of occurrences of this token in the index
        uint64_t document_freq; // Number of documents in which this token occurs.
        uint64_t size; // Size of the list for skipping over collisions.
        char type; // Type of token: end-of-document, word in anchor, URL, title or body.
        // Seek List
        // Type specific data?  
        UnrolledLinkList<Post> posting_list;


};

struct Post
{
    uint32_t delta;
    // Type specific data?
};
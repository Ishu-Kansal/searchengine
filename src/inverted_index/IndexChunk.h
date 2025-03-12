#include<vector>

#include "InvertedIndex.h"
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


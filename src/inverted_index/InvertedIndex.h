/*
Inverted Index Header File
*/
#include<vector>
#include "HashTable/HashTableStarterFiles/HashTable.h"

class InvertedIndex {

    public:
    private:
        HashTable<std::string, uint64_t> dictionary;
        std::vector<uint32_t> posting_list;

};


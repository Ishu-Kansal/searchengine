#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdio>
#include "../../utils/utf_encoding.h"
#include "Index.h"
#include "IndexFile.h"
#include "IndexFileReader.h"

bool fileExists(const std::string &filename) {
    std::ifstream file(filename);
    return file.good();
}

void basicIndexFileTest() {
    // Just checks if files get created
    IndexChunk indexChunk;
    std::string url = "http://example.com";
    size_t rank = 5;
    indexChunk.add_url(url, rank);

    std::string word1 = "test";
    indexChunk.add_word(word1, false);

    std::string word2 = "creeper";
    indexChunk.add_word(word2, true);

    indexChunk.add_enddoc();

    assert(!indexChunk.get_posting_lists().empty());

    uint32_t chunkNum = 1;
    IndexFile indexFile(chunkNum, indexChunk);

    char indexFilename[32];
    snprintf(indexFilename, sizeof(indexFilename), "IndexChunk_%05u", chunkNum);
    
    char hashFilename[32];
    snprintf(hashFilename, sizeof(hashFilename), "HashFile_%05u", chunkNum);

    assert(fileExists(indexFilename) && "Index file not created");
    assert(fileExists(hashFilename) && "Hash file not created");
    std::remove(indexFilename);
    std::remove(hashFilename);
    std::cout << "basicIndexFileTest passed." << std::endl;
}

void oneDocMultipleWordTest() {
    IndexChunk indexChunk;
    std::string url = "http://example.com";
    size_t rank = 1;
    indexChunk.add_url(url, rank);
    
    std::string wordApple = "apple";
    std::string wordBanana = "banana";

    indexChunk.add_word(wordApple, false);
    indexChunk.add_word(wordBanana, false);
    indexChunk.add_word(wordApple, false);
    indexChunk.add_word(wordApple, false);
    indexChunk.add_word(wordBanana, false);
    indexChunk.add_enddoc();

    assert(!indexChunk.get_posting_lists().empty());
    uint32_t chunkNum = 0;
    IndexFile indexFile(chunkNum, indexChunk);

    IndexFileReader reader(1);

    auto seekApple = reader.Find("apple", 0, chunkNum);
    assert(seekApple->location == 0);
    assert(seekApple != nullptr && "apple not found by IndexFileReader");
    delete seekApple;

    seekApple = reader.Find("apple", 1, chunkNum);
    assert(seekApple->location == 2);
    assert(seekApple != nullptr && "apple not found by IndexFileReader");
    delete seekApple;

    seekApple = reader.Find("apple", 2, chunkNum);
    assert(seekApple->location == 2);
    assert(seekApple != nullptr && "apple not found by IndexFileReader");
    delete seekApple;

    seekApple = reader.Find("apple", 3, chunkNum);
    assert(seekApple->location == 3);
    assert(seekApple != nullptr && "apple not found by IndexFileReader");
    delete seekApple;

    auto seekBanana = reader.Find("banana", 0, chunkNum);
    assert(seekBanana != nullptr && "banana not found by IndexFileReader");
    delete seekBanana;
    
    char indexFilename[32];
    snprintf(indexFilename, sizeof(indexFilename), "IndexChunk_%05u", chunkNum);
    
    char hashFilename[32];
    snprintf(hashFilename, sizeof(hashFilename), "HashFile_%05u", chunkNum);

    assert(fileExists(indexFilename) && "Index file not created");
    assert(fileExists(hashFilename) && "Hash file not created");

    std::remove(indexFilename);
    std::remove(hashFilename);
    std::cout << "oneDocMultipleWordTest passed." << std::endl;
}

void oneDocOneWordLoopTest() {
    IndexChunk indexChunk;
    std::string url = "http://example.com";
    size_t rank = 1;
    indexChunk.add_url(url, rank);
    
    std::string wordApple = "apple";

    for(int i = 0; i < (1 << 14); ++i)
    {
        indexChunk.add_word(wordApple, false);
    }
    indexChunk.add_enddoc();

    assert(!indexChunk.get_posting_lists().empty());
    uint32_t chunkNum = 0;
    IndexFile indexFile(chunkNum, indexChunk);

    IndexFileReader reader(1);

    auto seekApple = reader.Find("apple", 0, chunkNum);
    assert(seekApple->location == 0);
    assert(seekApple != nullptr && "apple not found by IndexFileReader");
    delete seekApple;

    seekApple = reader.Find("apple", 8192, chunkNum);
    assert(seekApple->location == 8192);
    assert(seekApple != nullptr && "apple not found by IndexFileReader");
    delete seekApple;

    seekApple = reader.Find("apple", 8193, chunkNum);
    assert(seekApple->location == 8193);
    assert(seekApple != nullptr && "apple not found by IndexFileReader");
    delete seekApple;

    seekApple = reader.Find("apple", 3, chunkNum);
    assert(seekApple->location == 3);
    assert(seekApple != nullptr && "apple not found by IndexFileReader");
    delete seekApple;

    auto seekBanana = reader.Find("banana", 0, chunkNum);
    assert(seekBanana == nullptr);
    delete seekBanana;
    
    char indexFilename[32];
    snprintf(indexFilename, sizeof(indexFilename), "IndexChunk_%05u", chunkNum);
    
    char hashFilename[32];
    snprintf(hashFilename, sizeof(hashFilename), "HashFile_%05u", chunkNum);

    assert(fileExists(indexFilename) && "Index file not created");
    assert(fileExists(hashFilename) && "Hash file not created");

    std::remove(indexFilename);
    std::remove(hashFilename);
    std::cout << "oneDocOneWordLoopTest passed." << std::endl;
}



int main() {
    //basicIndexFileTest();
    //oneDocMultipleWordTest();
    oneDocOneWordLoopTest();
    std::cout << "All tests passed." << std::endl;
    return 0;
}
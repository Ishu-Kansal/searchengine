#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdio>
#include "../../utils/utf_encoding.h"
#include "Index.h"
#include "IndexFile.h"
#include "IndexFileReader.h"
#include "../isr/isr.h"

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
    assert(seekApple!=nullptr && "apple not found by IndexFileReader");

    seekApple = reader.Find("apple", 1, chunkNum);
    assert(seekApple->location == 2);
    assert(seekApple!=nullptr && "apple not found by IndexFileReader");


    seekApple = reader.Find("apple", 2, chunkNum);
    assert(seekApple->location == 2);
    assert(seekApple!=nullptr && "apple not found by IndexFileReader");


    seekApple = reader.Find("apple", 3, chunkNum);
    assert(seekApple->location == 3);
    assert(seekApple!=nullptr && "apple not found by IndexFileReader");


    auto seekBanana = reader.Find("banana", 0, chunkNum);
    assert(seekBanana!=nullptr && "banana not found by IndexFileReader");

    
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
    assert(seekApple!=nullptr && "apple not found by IndexFileReader");

    seekApple = reader.Find("apple", 8192, chunkNum);
    assert(seekApple->location == 8192);
    assert(seekApple!=nullptr && "apple not found by IndexFileReader");

    seekApple = reader.Find("apple", 8193, chunkNum);
    assert(seekApple->location == 8193);
    assert(seekApple!=nullptr && "apple not found by IndexFileReader");

    seekApple = reader.Find("apple", 3, chunkNum);
    assert(seekApple->location == 3);
    assert(seekApple!=nullptr && "apple not found by IndexFileReader");

    auto seekBanana = reader.Find("banana", 0, chunkNum);
    assert(!seekBanana);
    
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
void seekTableOffsetTest() {
    IndexChunk indexChunk;
    std::string url = "http://example.com";
    size_t rank = 1;
    indexChunk.add_url(url, rank);
    
    std::string wordApple = "apple";
    std::string word = "word";

    for(int i = 0; i < 8192; ++i)
    {
        indexChunk.add_word(wordApple, false); // location from 0 to 8191
    }
    indexChunk.add_word(word, false); // location 8192
    indexChunk.add_word(wordApple, false); // location 8193
    indexChunk.add_enddoc();

    assert(!indexChunk.get_posting_lists().empty());
    uint32_t chunkNum = 0;
    IndexFile indexFile(chunkNum, indexChunk);

    IndexFileReader reader(1);
    for(int i = 0; i < 8192; ++i)
    {
        if (i == 8191) {
            int t =0;
        }
        std::cout << i << '\n';
        auto seekApple = reader.Find("apple", i, chunkNum);
        assert(seekApple->location == i);
        assert(seekApple->index == i);
        assert(seekApple!=nullptr && "apple not found by IndexFileReader");

    }
    auto seekApple = reader.Find("apple", 8191, chunkNum);
    assert(seekApple->location == 8191);
    assert(seekApple->index == 8191);
    assert(seekApple!=nullptr && "apple not found by IndexFileReader");


    seekApple = reader.Find("apple", 8192, chunkNum);
    assert(seekApple->location == 8193);
    assert(seekApple->index == 8192);
    assert(seekApple && "apple not found by IndexFileReader");


    seekApple = reader.Find("apple", 8193, chunkNum);
    assert(seekApple->location == 8193);
    assert(seekApple->index == 8192);
    assert(seekApple && "apple not found by IndexFileReader");

    
    char indexFilename[32];
    snprintf(indexFilename, sizeof(indexFilename), "IndexChunk_%05u", chunkNum);
    
    char hashFilename[32];
    snprintf(hashFilename, sizeof(hashFilename), "HashFile_%05u", chunkNum);

    assert(fileExists(indexFilename) && "Index file not created");
    assert(fileExists(hashFilename) && "Hash file not created");

    std::remove(indexFilename);
    std::remove(hashFilename);
    std::cout << "seekTableOffsetTest passed." << std::endl;
}

void urlListNoSeekTableTest() {
    IndexChunk indexChunk;
    for (int i = 0; i < 1000; ++i) 
    {
        std::string url = "http://example.com/" + std::to_string(i);
        size_t rank = 1;
        indexChunk.add_url(url, rank);
    }
    uint32_t chunkNum = 0;
    IndexFile indexFile(chunkNum, indexChunk);

    IndexFileReader reader(1);
    auto docObj = reader.FindUrl(0, 0);
    docObj = reader.FindUrl(5, 0);
}
void urlListTest() {
    IndexChunk indexChunk;
    for (int i = 0; i < 8193; ++i) 
    {
        std::string url = "http://example.com/" + std::to_string(i);
        size_t rank = 1;
        indexChunk.add_url(url, rank);
    }
    uint32_t chunkNum = 0;
    IndexFile indexFile(chunkNum, indexChunk);

    IndexFileReader reader(1);
    auto docObj = reader.FindUrl(0, 0);
    docObj = reader.FindUrl(8191, 0);
    docObj = reader.FindUrl(8192, 0);
}

void AndISRTest() {
    IndexChunk indexChunk;
    std::string wordApple = "apple";
    std::string wordTest = "test";
    std::string word = "word";
    for (int i = 0; i < 8193; ++i) 
    {
        std::string url = "http://example.com/" + std::to_string(i);
        indexChunk.add_url(url, i % 250);
        if (i % 5 == 0)
        {
            indexChunk.add_word(word, false);
            indexChunk.add_word(wordApple, false);
            indexChunk.add_word(word, false);
        }
        else 
        {
            indexChunk.add_word(word, false);
            indexChunk.add_word(word, false);
            indexChunk.add_word(wordTest, false);   
        }
 
        indexChunk.add_enddoc(); 
    }

    uint32_t chunkNum = 0;
    IndexFile indexFile(chunkNum, indexChunk);

    const IndexFileReader reader(1);
    
    unique_ptr<ISR> appleISR = make_unique<ISRWord>(wordApple, reader);
    unique_ptr<ISR> wordISR = make_unique<ISRWord>(word, reader);
    unique_ptr<ISR> testISR = make_unique<ISRWord>(wordTest, reader);

    vector<unique_ptr<ISR>> terms;

    terms.push_back(std::move(appleISR));
    terms.push_back(std::move(wordISR));

    ISRAnd andISR(std::move(terms), reader);

    for (int i = 0; i < 10; ++i)
    {
        auto x = andISR.Next();

    }

    vector<unique_ptr<ISR>> terms2;
    terms2.push_back(std::move(testISR));
    ISRAnd oneWordAnd(std::move(terms2), reader);
    for (int i = 0; i < 10; ++i)
    {
        auto y = oneWordAnd.Next();
    }
    oneWordAnd.Next();
    char indexFilename[32];
    snprintf(indexFilename, sizeof(indexFilename), "IndexChunk_%05u", chunkNum);
    
    char hashFilename[32];
    snprintf(hashFilename, sizeof(hashFilename), "HashFile_%05u", chunkNum);

    std::remove(indexFilename);
    std::remove(hashFilename);
}

int main() {
    // basicIndexFileTest();
    oneDocMultipleWordTest();
    oneDocOneWordLoopTest();
    seekTableOffsetTest();
    urlListNoSeekTableTest();
    urlListTest();
    AndISRTest();
    std::cout << "All tests passed." << std::endl;
    return 0;
}
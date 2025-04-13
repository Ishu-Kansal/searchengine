#include <vector>
#include "../utils/cstring_view.h"
#include <string_view>
#include "constraint_solver.h"
#include "query_parser/parser.h"
#include "inverted_index/Index.h"
#include <iostream>
#include <string>
#include <fstream>


extern std::vector<vector<ISRWord*>> sequences;

// driver function for the search engine
std::vector<string_view> run_engine(std::string& query) {
    QueryParser parser(query);
    Constraint *c = parser.Parse();
 
    if (c) {
       ISR* isrs = c->Eval();
       std::vector<UrlRank> raw_results = constraint_solver(isrs, sequences, 1);
 
       std::vector<std::string_view> urls;
       urls.reserve(raw_results.size());
       std::transform(raw_results.begin(), raw_results.end(), std::back_inserter(urls),
                    [](const UrlRank& p) { return p.url; });

 
       return urls;
 
    } 
    return {}; 
}

bool fileExists(const std::string &filename) {
    std::ifstream file(filename);
    return file.good();
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
    std::string banana = "banana";
    indexChunk.add_word(banana, false);
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

    std::cout << "oneDocOneWordLoopTest passed." << std::endl;
}

int main() {
    oneDocOneWordLoopTest(); 
    std::string test_string = "banana"; 
    assert(!run_engine(test_string).empty()); 
    std::cout << "All tests passed." << std::endl;
    return 0;
}
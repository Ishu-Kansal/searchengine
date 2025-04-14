#include <vector>
#include "../utils/cstring_view.h"
#include <string_view>
#include "constraint_solver.h"
#include "query_parser/parser.h"
#include "inverted_index/Index.h"
#include <iostream>
#include <string>
#include <fstream>
#include "globals.h"


// extern std::vector<std::vector<std::unique_ptr<ISRWord>>> sequences;

// driver function for the search engine
std::vector<string_view> run_engine(std::string& query) {

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

    QueryParser parser(query);
    std::unique_ptr<Constraint> c = parser.Parse();
 
    if (c) {
       std::unique_ptr<ISR> isrs = c->Eval();
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

int main() {
    std::string test_string = "banana"; 
    assert(!run_engine(test_string).empty()); 
    std::cout << "All tests passed." << std::endl;
    return 0;
}
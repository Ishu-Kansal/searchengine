#include <vector>
#include "../utils/cstring_view.h"
#include <string_view>
#include "constraint_solver.h"
#include "query_parser/parser.h"
#include "inverted_index/Index.h"
#include <iostream>
#include <string>
#include <fstream>


// driver function for the search engine
std::vector<string_view> run_engine(std::string& query) {

    uint32_t numChunks = 1;
    std::vector<std::vector<std::unique_ptr<ISRWord>>> sequences;
    IndexChunk indexChunk;
    std::string url = "http://example.com";
    size_t rank = 1;
    indexChunk.add_url(url, rank);
    
    std::string wordApple = "apple";

    for(int i = 0; i < (4); ++i)
    {
        indexChunk.add_word(wordApple, false);
    }
    std::string banana = "banana";
    indexChunk.add_word(banana, false);
    indexChunk.add_enddoc();

    assert(!indexChunk.get_posting_lists().empty());
    uint32_t chunkNum = 0;
    IndexFile indexFile(chunkNum, indexChunk);

    IndexFileReader reader(numChunks);

    QueryParser parser(query, numChunks, reader);
    std::unique_ptr<Constraint> c = parser.Parse();
 
    if (c) {
       std::unique_ptr<ISR> isrs = c->Eval(sequences);

        std::cout << "Sequences size: " << sequences.size() << std::endl;

       std::vector<UrlRank> raw_results = constraint_solver(isrs, sequences, 1, reader);
 
       std::vector<std::string_view> urls;
       urls.reserve(raw_results.size());
       std::transform(raw_results.begin(), raw_results.end(), std::back_inserter(urls),
                    [](const UrlRank& p) { return p.url; });

       return urls;
 
    } 
    return {}; 
}

int main() {
    std::string test_string = "banana"; 
    assert(!run_engine(test_string).empty()); 
    std::cout << "All tests passed." << std::endl;
    return 0;
}
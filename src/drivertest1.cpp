#include <vector>
#include <string_view>
#include <string>
#include <fstream> 
#include <iostream>
#include <memory>   
#include <stdexcept> 
#include <algorithm> 
#include <cassert>   

#include "../utils/cstring_view.h"
#include "constraint_solver.h"
#include "query_parser/parser.h"
#include "inverted_index/Index.h"
#include "json.hpp"

using json = nlohmann::json;

bool build_index_from_file(const std::string& filename, IndexChunk& indexChunk) 
{
    std::ifstream infile(filename);
    if (!infile.is_open())
    {
        return false;
    }

    std::string line;
    size_t line_num = 0;
    std::cout << "Building index from " << filename << "..." << std::endl;
    int i = 0;
    int apple = 0;
    uint64_t pos = 0;
    while (std::getline(infile, line))
    {
        line_num++;
    
        if (line.empty())
        {
            continue;
        }

        json j = json::parse(line);

        if (!j.contains("url") || !j["url"].is_string() ||
            !j.contains("rank") || !j["rank"].is_number_integer() ||
            !j.contains("content") || !j["content"].is_array())
        {
            continue;
        }

        std::string url = j["url"].get<std::string>();
        size_t rank = j["rank"].get<size_t>();

        bool found = false;
        indexChunk.add_url(url, rank);
        ++i;
        const auto& content_array = j["content"];
        for (const auto& item : content_array)
        {
            if (item.is_string())
            {
                std::string word = item.get<std::string>();
                if (word == "apple" && !found)
                {
                    found = true;
                    apple += 1;
                }
                indexChunk.add_word(word, false);
                pos++;
            }
        }
        found = false;
        indexChunk.add_enddoc();
        ++pos;

        if (i % 10000 == 0)
        {
             std::cout << "  Processed " << i << " documents..." << std::endl;
        }
    }

    std::cout << "Finished building index. Processed " << apple << " lines." << std::endl;
    infile.close();
    return true;
}

std::vector<UrlRank> run_engine(
    std::string& query,
    uint32_t numChunks, 
    IndexFileReader& reader 
) {
    std::vector<std::vector<std::unique_ptr<ISRWord>>> sequences;

    QueryParser parser(query, numChunks, reader);
    std::unique_ptr<Constraint> constraint = parser.Parse();

    if (constraint)
    {
        std::unique_ptr<ISR> isrs = constraint->Eval(sequences);

        if (!isrs)
        {
            std::cerr << "Warning: Constraint evaluation returned null ISR." << std::endl;
            return {};
        }

        std::vector<UrlRank> raw_results = constraint_solver(isrs, sequences, numChunks, reader);

        return raw_results;
    }
    else
    {
        std::cerr << "Query parsing failed for: " << query << std::endl;
        return {};
    }
}

int main()
{
    const std::string data_filename = "websites_data.jsonl";
    const uint32_t numChunks = 10; 

    const uint32_t chunkNum = 0; 

    /*
    IndexChunk indexChunk; 
    if (!build_index_from_file(data_filename, indexChunk)) 
    {
        return 1;
    }

    IndexFile indexFile(chunkNum, indexChunk); 

    std::cout << "Creating IndexFileReader..." << std::endl;
    */
    IndexFileReader reader(numChunks); 

    std::string test_query_1 = "umich engineering"; 
    std::cout << "\n--- Running Query 1: [" << test_query_1 << "] ---" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<UrlRank> results1 = run_engine(test_query_1, numChunks, reader);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Query 1 execution time: " << elapsed.count() << " seconds" << std::endl;

    if (!results1.empty())
    {
        std::cout << "Found " << results1.size() << " results for query 1 (showing top " << results1.size() << "):" << std::endl;
        for (const auto& url_sv : results1)
        {
            std::cout << "  Rank: " << url_sv.rank << " - " << url_sv.url << std::endl;
        }
    }

    return 0;
}
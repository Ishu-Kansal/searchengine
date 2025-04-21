#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../utils/cstring_view.h"
#include "constraint_solver.h"
#include "inverted_index/Index.h"
#include "json.hpp"
#include "query_parser/parser.h"

#include "../HtmlParser/HtmlParser.h"
#include "crawler/sockets.h"

static constexpr std::string_view APOS_ENTITY = "&#039;";
static constexpr std::string_view APOS_ENTITY2 = "&apos;";
static constexpr std::string_view HTML_ENTITY = "&#";

static constexpr std::string_view UNWANTED_NBSP = "&nbsp";
static constexpr std::string_view UNWANTED_LRM  = "&lrm";
static constexpr std::string_view UNWANTED_RLM  = "&rlm";
static constexpr size_t MAX_WORD_LENGTH = 50;

constexpr uint32_t MAX_PROCESSED = 10000;
constexpr uint32_t MAX_VECTOR_SIZE = 20000;
constexpr uint32_t MAX_QUEUE_SIZE = 100000;
constexpr uint32_t TOP_K_ELEMENTS = 7500;
constexpr uint32_t NUM_RANDOM = 10000;
constexpr uint32_t NUM_CHUNKS = 5;

bool isEnglish(const std::string &s) {
    for (const auto &ch : s) {
        if (static_cast<unsigned char>(ch) > 127) {
            return false;
        }
    }
    return true;
  }
  void cleanString(std::string &s) {
      // Gets rid of both &#039; and &apos;
      size_t pos = 0;
      while ((pos = s.find(APOS_ENTITY, pos)) != std::string::npos || 
             (pos = s.find(APOS_ENTITY2, pos)) != std::string::npos) {
          s.erase(pos, (pos == s.find(APOS_ENTITY, pos)) ? APOS_ENTITY.length() : APOS_ENTITY2.length());
      }
    if (s.size() > MAX_WORD_LENGTH)
    {
      s.clear();
      return;
    }
    // Gets rid of strings containing HTML entities
    if (s.find(HTML_ENTITY) != std::string::npos || s.find(UNWANTED_NBSP) != std::string::npos ||
    s.find(UNWANTED_LRM) != std::string::npos ||
    s.find(UNWANTED_RLM) != std::string::npos) 
    {
      s.clear();
      return;
    }
    // Gets rid of non english words
    /*
      if (!isEnglish(s)) {
      s.clear();
      return;
    }
    */
  
    // Gets rid of '
    s.erase(std::remove(s.begin(), s.end(), '\''), s.end());
  
    // Get rid of start and end non punctuation
    auto start = std::find_if(s.begin(), s.end(), ::isalnum);
      
    if (start == s.end()) {
        s.clear();
        return;
    }
    
    auto end = std::find_if(s.rbegin(), s.rend(), ::isalnum).base();
    
    s.erase(end, s.end());
    s.erase(s.begin(), start);
  }
  
  std::vector<std::string> splitHyphenWords(const std::string &word) {
    std::vector<std::string> parts;
    if (word.find('-') != std::string::npos) {
        size_t start = 0, end = 0;
        while ((end = word.find('-', start)) != std::string::npos) {
            std::string token = word.substr(start, end - start);
            if (!token.empty())
                parts.push_back(token);
            start = end + 1;
        }
        std::string token = word.substr(start);
        if (!token.empty())
            parts.push_back(token);
    } else {
        parts.push_back(word);
    }
    return parts;
  }

  
using json = nlohmann::json;

bool build_index_from_file(const std::string& filename,
                           IndexChunk& indexChunk) {
  std::ifstream infile(filename);
  if (!infile.is_open()) {
    return false;
  }

  std::string line;
  size_t line_num = 0;
  std::cout << "Building index from " << filename << "..." << std::endl;
  int i = 0;
  int apple = 0;
  uint64_t pos = 0;
  while (std::getline(infile, line)) {
    line_num++;

    if (line.empty()) {
      continue;
    }

    json j = json::parse(line);

    std::string url = j["url"].get<std::string>();
    size_t rank = j["rank"].get<size_t>();

    bool found = false;
    indexChunk.add_url(url, rank);
    ++i;
    const auto& content_array = j["content"];
    for (const auto& item : content_array) {
      if (item.is_string()) {
        std::string word = item.get<std::string>();
        if (word == "apple" && !found) {
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

    if (i % 10000 == 0) {
      std::cout << "  Processed " << i << " documents..." << std::endl;
    }
  }

  std::cout << "Finished building index. Processed " << apple << " lines."
            << std::endl;
  infile.close();
  return true;
}

std::vector<UrlRank> run_engine(std::string& query, uint32_t numChunks,
                                IndexFileReader& reader, int& matches) {
  std::vector<std::vector<std::unique_ptr<ISRWord>>> sequences;

  QueryParser parser(query, numChunks, reader);
  std::unique_ptr<Constraint> constraint = parser.Parse();

  if (constraint) {
    std::unique_ptr<ISR> isrs = constraint->Eval(sequences);

    if (!isrs) {
      std::cerr << "Warning: Constraint evaluation returned null ISR."
                << std::endl;
      return {};
    }

    matches = 0;
    std::vector<UrlRank> raw_results =
        constraint_solver(isrs, sequences, numChunks, reader, matches);

    return raw_results;
  } else {
    std::cerr << "Query parsing failed for: " << query << std::endl;
    return {};
  }
}

int main(int argc, char** argv) {
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

  int matches = 0;

  std::string test_query_1 = argc > 1 ? argv[1] : "umich engineering";
  std::cout << "\n--- Running Query 1: [" << test_query_1 << "] ---"
            << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  std::vector<UrlRank> results1 = run_engine(test_query_1, numChunks, reader, matches);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  std::cout << "Query 1 execution time: " << elapsed.count() << " seconds"
            << std::endl;

  if (!results1.empty()) {
    std::cout << "Found " << matches
              << " results for query 1 (showing top " << results1.size()
              << "):" << std::endl;
    for (const auto& url_sv : results1) {
      std::cout << "  Rank: " << url_sv.rank << " - " << url_sv.url
                << std::endl;
    }
  }



  return 0;
}

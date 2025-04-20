#include <vector>
#include <string_view>
#include <string>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <cassert>

#include "driver.h"
#include "crawler/sockets.h"
#include "../utils/cstring_view.h"
#include "constraint_solver.h"
#include "query_parser/parser.h"
#include "inverted_index/Index.h"
#include "../HtmlParser/HtmlParser.h"
#include "json.hpp"

#include <thread>
#include <chrono>

using json = nlohmann::json;

Driver::Driver() {}

// Joins up to max_words into a space-separated string
std::string Driver::join_words(const std::vector<std::string>& words, size_t max_words) {
   std::ostringstream oss;
   size_t count = std::min(max_words, words.size());
   for (size_t i = 0; i < count; ++i) {
       oss << words[i];
       if (i != count - 1) oss << ' ';
   }
   return oss.str();
}

// Given a URL, fetch its HTML content and parse it into a SearchResult struct containing:
// - the raw URL,
// - a parsed title,
// - and a snippet (short preview of the page content).
SearchResult Driver::get_url_and_parse(const std::string& url) {
   std::string url_str(url.data(), url.size());
   std::string html;

   int status = getHTML(url_str, html);
   SearchResult result{url_str, "", ""};

   if (status != 0 || html.empty()) {
       std::cerr << "Failed to fetch HTML from: " << url_str << " status: " << status << std::endl;
       return result;
   }

   try {
       HtmlParser parser(html.data(), html.size());
       result.title = join_words(parser.titleWords);
       result.snippet = join_words(parser.description, 30);
   }
   catch (...) {
       std::cerr << "Parsing failed for: " << url_str << std::endl;
   }

   return result;
}


std::vector<UrlRank> run_engine_helper(
    std::string &query,
    uint32_t numChunks,
    IndexFileReader &reader)
{
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

std::vector<std::string> Driver::run_engine(std::string& query) {
   const std::string data_filename = "websites_data.jsonl";
   const uint32_t numChunks = 1;

   const uint32_t chunkNum = 0;

   IndexFileReader reader(numChunks);

   std::cout << "\n--- Running Query: [" << query << "] ---" << std::endl;
   auto start = std::chrono::high_resolution_clock::now();
   std::vector<UrlRank> results = run_engine_helper(query, numChunks, reader);
   auto end = std::chrono::high_resolution_clock::now();
   std::chrono::duration<double> elapsed = end - start;
   std::cout << "Query execution time: " << elapsed.count() << " seconds" << std::endl;

   std::vector<std::string> urls;
   if (!results.empty())
   {
      std::cout << "Found " << results.size() << " results for query (showing top " << results.size() << "):" << std::endl;
      for (const auto &url_sv : results)
      {
         std::cout << "  Rank: " << url_sv.rank << " - " << url_sv.url << std::endl;
         urls.emplace_back(url_sv.url);
      }
   }

   return urls;
   

   // std::vector<std::string_view> urls;

   // urls.push_back(string_view("https://www.nationalgeographic.org/"));
   // urls.push_back(string_view("https://www.amnh.org/"));
   // urls.push_back(string_view("https://www.metmuseum.org/"));
   // urls.push_back(string_view("https://www.moma.org/"));
   // urls.push_back(string_view("https://www.weather.gov/"));
   // urls.push_back(string_view("https://www.accuweather.com/"));
   // urls.push_back(string_view("https://www.wunderground.com/"));
   // urls.push_back(string_view("https://www.nist.gov/"));
   // urls.push_back(string_view("https://www.ieee.org/"));
   // urls.push_back(string_view("https://www.acm.org/"));
   // urls.push_back(string_view("https://www.ucla.edu/"));
   // urls.push_back(string_view("https://www.utexas.edu/"));
   // urls.push_back(string_view("https://www.psu.edu/"));
   // urls.push_back(string_view("https://www.cornell.edu/"));
   // urls.push_back(string_view("https://www.duke.edu/"));
   // urls.push_back(string_view("https://www.jhu.edu/"));
   // urls.push_back(string_view("https://www.northwestern.edu/"));
   // urls.push_back(string_view("https://www.uchicago.edu/"));

   // // DELETE THIS LATER - simulate constraint solver taking 3 seconds
   // std::this_thread::sleep_for(std::chrono::seconds(3));

   // return urls;
}
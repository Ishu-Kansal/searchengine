// driver.cpp
// This file implements the main logic for the search engine backend.
// It defines how queries are parsed, how matching URLs are fetched and parsed for content,
// and how title/snippet results are prepared for use by the front-end in engine.cpp.
// It connects the query parser, HTML fetcher, and HTML parser modules into a coherent workflow.

#include "driver.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <memory>
#include <algorithm>

#include "constraint_solver.h"
#include "query_parser/parser.h"
#include "inverted_index/Index.h"
#include "../HtmlParser/HtmlParser.h"
#include "./crawler/sockets.h"


Driver::Driver() { }

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
SearchResult Driver::get_url_and_parse(const std::string_view& url) {
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
   } catch (...) {
       std::cerr << "Parsing failed for: " << url_str << std::endl;
   }

   return result;
}


std::vector<std::string_view> Driver::run_engine(std::string& query) {
//    uint32_t numChunks = 1;
//    std::vector<std::vector<std::unique_ptr<ISRWord>>> sequences;

//    // Example document manually inserted
//    IndexChunk indexChunk;
//    std::string url = "http://example.com";
//    std::string wordApple = "apple";
//    std::string banana = "banana";

//    indexChunk.add_url(url, 1);
//    for (int i = 0; i < 4; ++i) {
//        indexChunk.add_word(wordApple, false);
//    }
//    indexChunk.add_word(banana, false);
//    indexChunk.add_enddoc();

//    assert(!indexChunk.get_posting_lists().empty());

//    IndexFile indexFile(0, indexChunk);
//    IndexFileReader reader(numChunks);

//    QueryParser parser(query, numChunks, reader);
//    std::unique_ptr<Constraint> c = parser.Parse();

//    if (!c) return {};

//    std::unique_ptr<ISR> isrs = c->Eval(sequences);
//    std::vector<UrlRank> raw_results = constraint_solver(isrs, sequences, 1, reader);

   std::vector<std::string_view> urls;
//    urls.reserve(raw_results.size());
//    std::transform(raw_results.begin(), raw_results.end(), std::back_inserter(urls),
//                   [](const UrlRank& p) { return p.url; });

//    return urls;
// }
   

    urls.push_back(string_view("https://www.nationalgeographic.org/"));
    urls.push_back(string_view("https://www.amnh.org/"));
    urls.push_back(string_view("https://www.metmuseum.org/"));
    urls.push_back(string_view("https://www.moma.org/"));
    urls.push_back(string_view("https://www.weather.gov/"));
    urls.push_back(string_view("https://www.accuweather.com/"));
    urls.push_back(string_view("https://www.wunderground.com/"));
    urls.push_back(string_view("https://www.nist.gov/"));
    urls.push_back(string_view("https://www.ieee.org/"));
    urls.push_back(string_view("https://www.acm.org/"));
    urls.push_back(string_view("https://www.ucla.edu/"));
    urls.push_back(string_view("https://www.utexas.edu/"));
    urls.push_back(string_view("https://www.psu.edu/"));
    urls.push_back(string_view("https://www.cornell.edu/"));
    urls.push_back(string_view("https://www.duke.edu/"));
    urls.push_back(string_view("https://www.jhu.edu/"));
    urls.push_back(string_view("https://www.northwestern.edu/"));
    urls.push_back(string_view("https://www.uchicago.edu/"));
    return urls;

}
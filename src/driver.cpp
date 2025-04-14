/*
 * driver.cpp
 *
 * Provides the core logic for the search engine backend.
 * - Defines how queries are processed into search results.
 * - Fetches and parses web pages using an HTML parser.
 *
 * Dependencies:
 * - ISR (Intermediate Search Representation)
 * - QueryParser: for parsing queries
 * - ConstraintSolver: for evaluating constraints
 * - HtmlParser: for parsing HTML titles and snippets
 * - sockets.h: for SSL-based HTML fetching using getHTML()
 * 
 * Output:
 * - Each query yields a list of SearchResult objects, with populated `url`, `title`, and `snippet`.
 * 
 * Note: This version includes placeholder/mock URLs in `run_engine()` for testing.
 */

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include "isr/isr.h"
#include <algorithm>

// #include "query_parser/parser.h"
// #include "constraint_solver.h"
#include "../HtmlParser/HtmlParser.h"
#include "./crawler/sockets.h"

extern std::vector<vector<ISRWord*>> sequences;

/**
 * @brief Struct to represent a single search result.
 * 
 * Holds the target URL, extracted page title, and a short snippet/description.
 */
struct SearchResult {
   std::string url;
   std::string title;
   std::string snippet;
};

/**
 * @brief Concatenates a vector of words into a space-separated string.
 * 
 * @param words Vector of string tokens to join.
 * @param max_words Optional limit on the number of words to join.
 * @return std::string Resulting space-separated string.
 */
std::string join_words(const std::vector<std::string>& words, size_t max_words = SIZE_MAX) {
   std::ostringstream oss;
   size_t count = std::min(max_words, words.size());
   for (size_t i = 0; i < count; ++i) {
       oss << words[i];
       if (i != count - 1) oss << ' ';
   }
   return oss.str();
}

/**
 * @brief Fetches and parses the HTML of a given URL.
 * 
 * Uses getHTML() to retrieve the content and HtmlParser to extract the title and snippet.
 * 
 * @param url The URL to fetch, as a `cstring_view`.
 * @return SearchResult Object containing the URL, title, and snippet.
 */
SearchResult get_and_parse_url(const cstring_view& url) {
   std::string url_str(url.data(), url.size());
   std::string html;

   int status = getHTML(url_str, html);
   SearchResult result;
   result.url = url_str;


   // Check if fetching was successful
    if (status != 0 || html.empty()) {
       std::cerr << "Failed to fetch HTML from: " << url_str << std::endl;
       result.title = "";
       result.snippet = "";
       return result;
    }

    // Parse the HTML content
    try {
       HtmlParser parser(html.data(), html.size());
       result.title = join_words(parser.titleWords);
       result.snippet = join_words(parser.description, 30);

    } catch (...) {
       std::cerr << "Parsing failed for: " << url_str << std::endl;
       result.title = "";
       result.snippet = "";
    }

   return result;
}

/**
 * @brief Main entry point for the search engine.
 * 
 * - Parse the query
 * - Use the ISR system to evaluate constraints
 * - Rank URLs based on relevance and other signals
 * 
 * @param query User input string to be processed.
 * @return std::vector<cstring_view> List of candidate URLs.
 */
std::vector<cstring_view> run_engine(std::string& query) {
    // QueryParser parser(query);
    // Constraint *c = parser.Parse();

    // if (c) {
    // ISR* isrs = c->Eval();
    // std::vector<UrlRank> raw_results = constraint_solver(isrs, sequences, 1);

    // std::vector<std::string_view> urls;
    // urls.reserve(raw_results.size());
    // std::transform(raw_results.begin(), raw_results.end(), std::back_inserter(urls),
    //                 [](const UrlRank& p) { return p.url; });


    // return urls;

    // } 
    // return {}; 
   std::vector<cstring_view> urls;
   urls.push_back(cstring_view("https://www.github.com/"));
   urls.push_back(cstring_view("https://www.rottentomatoes.com/"));
   urls.push_back(cstring_view("https://www.merriam-webster.com/"));
   urls.push_back(cstring_view("https://www.wikihow.com/"));
   urls.push_back(cstring_view("https://www.caranddriver.com/"));
   urls.push_back(cstring_view("https://www.zillow.com/"));
   urls.push_back(cstring_view("https://www.quora.com/"));
   urls.push_back(cstring_view("https://medium.com/"));
   urls.push_back(cstring_view("https://www.eff.org/"));
   urls.push_back(cstring_view("https://www.w3.org/"));
   urls.push_back(cstring_view("https://www.ietf.org/"));
   urls.push_back(cstring_view("https://kernel.org/"));
   urls.push_back(cstring_view("https://www.apache.org/"));
   urls.push_back(cstring_view("https://www.python.org/"));

   return urls;
}

// TO DO: Wrap everything in a main function
int main() {
   return -1;
}

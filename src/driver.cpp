#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>

// #include "query_parser/parser.h"
// #include "constraint_solver.h"
#include "../HtmlParser/HtmlParser.h"
#include "./crawler/sockets.h"

//std::vector<vector<ISRWord*>> sequences;

struct SearchResult {
   std::string url;
   std::string title;
   std::string snippet;
};

// Helper to join vector<string> with spaces
std::string join_words(const std::vector<std::string>& words, size_t max_words = SIZE_MAX) {
   std::ostringstream oss;
   size_t count = std::min(max_words, words.size());
   for (size_t i = 0; i < count; ++i) {
       oss << words[i];
       if (i != count - 1) oss << ' ';
   }
   return oss.str();
}

// Fetch content from URL using LinuxGetSsl and return SearchResult
SearchResult get_and_parse_url(const cstring_view& url) {
   std::string url_str(url.data(), url.size());
   std::string html;

   int status = getHTML(url_str, html);
   SearchResult result;
   result.url = url_str;
   std::cout << "Status: " << status << std::endl;
   std::cout << "HTML length: " << html.length() << std::endl;


   if (status != 0 || html.empty()) {
      std::cout << status << std::endl;
       std::cerr << "Failed to fetch HTML from: " << url_str << std::endl;
       result.title = "";
       result.snippet = "";
       return result;
   }

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

// driver function for the search engine
std::vector<cstring_view> run_engine(std::string& query) {
   // QueryParser parser(query);
   // Constraint *c = parser.Parse();

   // if (c) {
   //    ISR* isrs = c->Eval();
   //    std::vector<std::pair<cstring_view, int>> raw_results = constraint_solver(isrs, sequences);

   //    std::vector<cstring_view> urls;
   //    urls.reserve(raw_results.size());
   //    std::transform(raw_results.begin(), raw_results.end(), std::back_inserter(urls),
   //                   [](const std::pair<cstring_view, int>& p) { return p.first; });

   //    return urls;

   // } else {
   //    return {};  // Return empty vector on failure
   // }
   std::vector<cstring_view> urls;
   urls.push_back(cstring_view("https://www.github.com/"));
   urls.push_back(cstring_view("https://www.example.com/"));
   urls.push_back(cstring_view("https://www.wikipedia.org/"));
   urls.push_back(cstring_view("https://www.stackoverflow.com/"));
   urls.push_back(cstring_view("https://www.reddit.com/"));

   return urls;
}

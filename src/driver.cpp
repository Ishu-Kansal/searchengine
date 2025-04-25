#include "driver.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "../HtmlParser/HtmlParser.h"
#include "../utils/cstring_view.h"
#include "constraint_solver.h"
#include "crawler/sockets.h"
#include "inverted_index/Index.h"
#include "json.hpp"
#include "query_parser/parser.h"

using json = nlohmann::json;

Driver::Driver() {}

// Joins up to max_words into a space-separated string
std::string Driver::join_words(const std::vector<std::string>& words,
                               size_t max_words) {
  std::ostringstream oss;
  size_t count = std::min(max_words, words.size());
  for (size_t i = 0; i < count; ++i) {
    oss << words[i];
    if (i != count - 1) oss << ' ';
  }
  return oss.str();
}

std::string Driver::decode_html_entities(const std::string& input) {
  static const std::unordered_map<std::string, std::string> named_entities = {
      {"amp", "&"},   {"lt", "<"},    {"gt", ">"},    {"quot", "\""},
      {"apos", "'"},  {"nbsp", " "},  {"copy", "©"},  {"reg", "®"},
      {"euro", "€"},  {"pound", "£"}, {"cent", "¢"},  {"yen", "¥"},
      {"deg", "°"},   {"sect", "§"},  {"para", "¶"},  {"hellip", "…"},
      {"mdash", "—"}, {"ndash", "-"}, {"lsquo", "'"}, {"rsquo", "'"},
      {"ldquo", "“"}, {"rdquo", "”"}, {"bull", "•"},  {"trade", "™"},
      {"iexcl", "¡"}};

  std::string output;
  size_t i = 0;
  while (i < input.length()) {
    if (input[i] == '&') {
      size_t semicolon = input.find(';', i + 1);
      if (semicolon != std::string::npos) {
        std::string entity = input.substr(i + 1, semicolon - i - 1);

        // Check for numeric entity
        if (!entity.empty() && entity[0] == '#') {
          char decoded_char = '?';  // fallback
          try {
            if (!entity.empty() && entity[0] == '#') {
              std::string decoded_char = "?";  // fallback
              try {
                int code = 0;
                if (entity[1] == 'x' || entity[1] == 'X') {
                  code = std::stoi(entity.substr(2), nullptr, 16);
                } else {
                  code = std::stoi(entity.substr(1));
                }

                // Convert to UTF-8
                if (code <= 0x7F) {
                  decoded_char = std::string(1, static_cast<char>(code));
                } else if (code <= 0x7FF) {
                  decoded_char = {static_cast<char>(0xC0 | (code >> 6)),
                                  static_cast<char>(0x80 | (code & 0x3F))};
                } else if (code <= 0xFFFF) {
                  decoded_char = {
                      static_cast<char>(0xE0 | (code >> 12)),
                      static_cast<char>(0x80 | ((code >> 6) & 0x3F)),
                      static_cast<char>(0x80 | (code & 0x3F))};
                } else if (code <= 0x10FFFF) {
                  decoded_char = {
                      static_cast<char>(0xF0 | (code >> 18)),
                      static_cast<char>(0x80 | ((code >> 12) & 0x3F)),
                      static_cast<char>(0x80 | ((code >> 6) & 0x3F)),
                      static_cast<char>(0x80 | (code & 0x3F))};
                }
              } catch (...) {
                decoded_char = "?";
              }
              output += decoded_char;
              i = semicolon + 1;
            }
          } catch (...) {
            decoded_char = '?';
          }
          output += decoded_char;
          i = semicolon + 1;
        } else if (named_entities.count(entity)) {
          output += named_entities.at(entity);
          i = semicolon + 1;
        } else {
          output += '&';  // unknown entity, keep as-is
          ++i;
        }
      } else {
        output += input[i];
        ++i;
      }
    } else {
      output += input[i];
      ++i;
    }
  }
  return output;
}

// Given a URL, fetch its HTML content and parse it into a SearchResult struct
// containing:
// - the raw URL,
// - a parsed title,
// - and a snippet (short preview of the page content).
SearchResult Driver::get_url_and_parse(const std::string& url) {
  std::string url_str(url.data(), url.size());
  std::string html;

  int status = getHTML(url_str, html);
  SearchResult result{url_str, "", ""};

  if (status != 0 || html.empty()) {
    std::cerr << "Failed to fetch HTML from: " << url_str
              << " status: " << status << std::endl;
    return result;
  }

  try {
    HtmlParser parser(html.data(), html.size());
    result.title = decode_html_entities(join_words(parser.titleWords));
    result.snippet = decode_html_entities(join_words(parser.description, 30));
  } catch (...) {
    std::cerr << "Parsing failed for: " << url_str << std::endl;
  }

  return result;
}

std::vector<UrlRank> run_engine_helper(std::string& query, uint32_t numChunks,
                                       IndexFileReader& reader, int& matches) {
  std::vector<std::vector<std::unique_ptr<ISRWord>>> sequences;

  QueryParser parser(query, numChunks, reader);
  std::unique_ptr<Constraint> constraint = parser.Parse();

<<<<<<< Updated upstream
   std::cout << "Calling parser" << std::endl;
   QueryParser parser(query, numChunks, reader);
   std::cout << "Out of parser" << std::endl;
   std::unique_ptr<Constraint> constraint = parser.Parse();

   if (constraint)
   {
      std::unique_ptr<ISR> isrs = constraint->Eval(sequences);

      if (!isrs)
      {
         std::cerr << "Warning: Constraint evaluation returned null ISR." << std::endl;
         return {};
      }

      int sequences_length = 0;
      for (auto& sequence : sequences)
      {
          sequences_length += sequence.size();
      }

      std::cout << "Calling constraint solver" << std::endl;
      std::vector<UrlRank> raw_results = constraint_solver(isrs, sequences, numChunks, reader, matches, sequences_length);

      return raw_results;
   }
   else
   {
      std::cerr << "Query parsing failed for: " << query << std::endl;
=======
  if (constraint) {
    std::unique_ptr<ISR> isrs = constraint->Eval(sequences);

    if (!isrs) {
      std::cerr << "Warning: Constraint evaluation returned null ISR."
                << std::endl;
>>>>>>> Stashed changes
      return {};
    }

    int sequences_length = 0;
    for (auto& sequence : sequences) {
      sequences_length += sequence.size();
    }

    std::vector<UrlRank> raw_results = constraint_solver(
        isrs, sequences, numChunks, reader, matches, sequences_length);

    return raw_results;
  } else {
    std::cerr << "Query parsing failed for: " << query << std::endl;
    return {};
  }
}

<<<<<<< Updated upstream
std::vector<std::pair<std::string, int>> Driver::run_engine(std::string& query, std::string& summary, IndexFileReader& reader) {
=======
<<<<<<< Updated upstream
std::vector<std::string> Driver::run_engine(std::string& query, std::string& summary, IndexFileReader& reader) {
>>>>>>> Stashed changes
   const std::string data_filename = "websites_data.jsonl";
   const uint32_t numChunks = 100;
=======
std::vector<std::string> Driver::run_engine(std::string& query,
                                            std::string& summary) {
  const std::string data_filename = "websites_data.jsonl";
  const uint32_t numChunks = 1010;
>>>>>>> Stashed changes

  const uint32_t chunkNum = 0;

<<<<<<< Updated upstream
   int matches = 0;
=======
  IndexFileReader reader(numChunks);

  int matches = 0;
>>>>>>> Stashed changes

<<<<<<< Updated upstream
   std::vector<std::pair<std::string, int>> urls;
   if (!results.empty())
   {
      std::cout << "Found " << matches << " results for query (showing top " << results.size() << "):" << std::endl;
      for (const auto &url_sv : results)
      {
         std::cout << "  Rank: " << url_sv.rank << " - " << url_sv.url << std::endl;
         urls.emplace_back(url_sv.url, url_sv.rank);
      }
   }
=======
  std::cout << "\n--- Running Query: [" << query << "] ---" << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  std::vector<UrlRank> results =
      run_engine_helper(query, numChunks, reader, matches);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  std::cout << "Query execution time: " << elapsed.count() << " seconds"
            << std::endl;
>>>>>>> Stashed changes

  std::vector<std::string> urls;
  if (!results.empty()) {
    std::cout << "Found " << matches << " results for query (showing top "
              << results.size() << "):" << std::endl;
    for (const auto& url_sv : results) {
      std::cout << "  Rank: " << url_sv.rank << " - " << url_sv.url
                << std::endl;
      urls.emplace_back(url_sv.url);
    }
  }

  std::ostringstream oss;
  oss << std::fixed << std::setprecision(3) << elapsed.count();
  summary = "Found " + std::to_string(matches) + " matches in " + oss.str() +
            " seconds";

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
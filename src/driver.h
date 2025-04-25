// driver.h
// Header file for driver.cpp

#pragma once

#include <string>
#include <vector>
#include "../src/inverted_index/IndexFileReader.h"

struct SearchResult {
    std::string url;
    std::string title;
    std::string snippet;
};

class Driver {
    public:
        Driver();
    
        // Runs the full query pipeline and returns the matching URLs
        std::vector<std::pair<std::string, int>> run_engine(std::string &query, std::string &summary, IndexFileReader &reader);

        // Given a URL, fetches and parses HTML into SearchResult metadata
        SearchResult get_url_and_parse(const std::string& url);
    
    private:
        // Utility for joining words into a space-separated string
        std::string join_words(const std::vector<std::string>& words, size_t max_words = SIZE_MAX);

        std::string decode_html_entities(const std::string& input);

    };
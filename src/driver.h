// driver.h
// Header file for driver.cpp

#pragma once

#include <string>
#include <vector>

struct SearchResult {
    std::string url;
    std::string title;
    std::string snippet;
};

class Driver {
    public:
        Driver();
    
        // Runs the full query pipeline and returns the matching URLs
        std::vector<std::string_view> run_engine(std::string& query);
    
        // Given a URL, fetches and parses HTML into SearchResult metadata
        SearchResult get_url_and_parse(const std::string_view& url);
    
    private:
        // Utility for joining words into a space-separated string
        std::string join_words(const std::vector<std::string>& words, size_t max_words = SIZE_MAX);

    };
#pragma once

#include <string>
#include <vector>
#include "../utils/cstring_view.h"

struct SearchResult {
    std::string url;
    std::string title;
    std::string snippet;
};

std::string join_words(const std::vector<std::string>& words, size_t max_words = SIZE_MAX);
SearchResult get_and_parse_url(const cstring_view& url);
std::vector<cstring_view> run_engine(std::string& query);

// engine.cpp
#include <string>
#include <pthread.h>
#include <iostream>
#include "Plugin.h"
#include "Mutex.h"
#include "json.hpp" // Download from https://github.com/nlohmann/json"

#include "../src/inverted_index/IndexFileReader.h"
#include "../src/driver.h"

using json = nlohmann::json;

json result;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Shared driver instance
Driver driver;

uint32_t numChunks = 100;
IndexFileReader reader(numChunks);

std::string clean_url(std::string& url) {
  std::string cleaned = url;

  // Remove prefix only once, in priority order
  const std::string https_prefix = "https://";
  const std::string http_prefix = "http://";
  const std::string www_prefix = "www.";

  if (cleaned.rfind(https_prefix, 0) == 0) {
    cleaned.erase(0, https_prefix.length());
  } else if (cleaned.rfind(http_prefix, 0) == 0) {
    cleaned.erase(0, http_prefix.length());
  } else if (cleaned.rfind(www_prefix, 0) == 0) {
    cleaned.erase(0, www_prefix.length());
  }

  // Then remove query string after '?'
  size_t query_pos = cleaned.find('?');
  if (query_pos != std::string::npos) {
    cleaned = cleaned.substr(0, query_pos);
  }

  return cleaned;
}


bool is_valid_utf8(const std::string& string) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(string.c_str());
    size_t len = string.length();
    size_t i = 0;

    while (i < len) {
        if (bytes[i] <= 0x7F) {
            i += 1;
        } else if ((bytes[i] & 0xE0) == 0xC0) {
            if (i + 1 >= len || (bytes[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if ((bytes[i] & 0xF0) == 0xE0) {
            if (i + 2 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if ((bytes[i] & 0xF8) == 0xF0) {
            if (i + 3 >= len || (bytes[i + 1] & 0xC0) != 0x80 ||
                (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80)
                return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}


// Struct used to pass URL and index info to each thread
struct ThreadData {
  std::string url;
  int idx;
};

// Thread worker function for fetching title and snippet
void* snippet_thread_worker(void* arg) {
  ThreadData* data = static_cast<ThreadData*>(arg);
  SearchResult parsed = driver.get_url_and_parse(data->url);

  pthread_mutex_lock(&result_mutex);

  std::string lowered_title = parsed.title;
  std::string lowered_snippet = parsed.snippet;
  std::transform(lowered_title.begin(), lowered_title.end(), lowered_title.begin(), ::tolower);
  std::transform(lowered_snippet.begin(), lowered_snippet.end(), lowered_snippet.begin(), ::tolower);

  bool should_clean_title = false;

  if (!is_valid_utf8(parsed.title) || parsed.title.empty()) {
      should_clean_title = true;
  } else if (lowered_title.find("page not found") != std::string::npos ||
             lowered_snippet.find("page not found") != std::string::npos) {
      should_clean_title = true;
  } else if (parsed.title.rfind("http", 0) == 0 || parsed.title.rfind("www", 0) == 0) {
      should_clean_title = true;
  }

  if (should_clean_title) {
      parsed.title = clean_url(parsed.url);
  }

  if (!is_valid_utf8(parsed.snippet) || lowered_snippet.find("page not found") != std::string::npos) {
      parsed.snippet = "";
  }

  result["results"][data->idx] = {
      {"url", parsed.url},
      {"title", parsed.title},
      {"snippet", parsed.snippet}
  };

  pthread_mutex_unlock(&result_mutex);
  delete data;
  return nullptr;
}



// Executes a query by calling the search backend (run_engine),
// then concurrently fetching the matching URLs.
// This modified version returns just the URLs.
json run_query(std::string &query) {
  std::cout << "In run_query" << std::endl;

  std::string summary;
  std::vector<std::string> urls = driver.run_engine(query, summary, reader);

  std::cout << "Finished run_engine" << std::endl;

  result["results"] = json::array();
  for (std::string& url : urls) {
    if (!is_valid_utf8(url)) {
      continue;
    }

    std::string cleaned = clean_url(url);

    result["results"].push_back({
        {"url", url},
        {"title", cleaned},
        {"snippet", ""}
    });
  }

  std::cout << "Summary: " << summary << std::endl;

  if (urls.size() == 0) {
    std::cout << "No results found" << std::endl;
  }

  result["summary"] = summary;

  return result;
}

// Executes fetching of snippets for a given list of URLs
json fetch_snippets(std::vector<std::string> &urls) {
  result["results"] = json::array();

  std::vector<pthread_t> threads;

  // Create a thread for each URL to fetch the snippet
  for (int i = 0; i < urls.size(); i++) {
      pthread_t thread;
      ThreadData* data = new ThreadData{urls[i], i};

      if (pthread_create(&thread, nullptr, snippet_thread_worker, data) != 0) {
          std::cerr << "Failed to create thread for URL: " << urls[i] << std::endl;
          delete data;
      } else {
          threads.push_back(thread);
      }
  }

  // Join all threads
  for (pthread_t& thread : threads) {
      pthread_join(thread, nullptr);
  }

  return result;
}

// Plugin class that handles HTTP requests routed to "/api/search/"
class SearchAPI : public PluginObject {
private:
  Mutex lock;
  const std::string search_endpoint = "/api/search/";
  const std::string snippets_endpoint = "/api/snippets/";

  // Handle search requests
  std::string handle_search(const std::string &req) {
    size_t pos = req.find("\r\n\r\n");
    if (pos == std::string::npos) return error(400);

    std::string body = req.substr(pos + 4);
    json j = json::parse(body, nullptr, false);
    if (j.is_discarded() || !j.contains("query")) return error(400);

    std::string q = j["query"].get<std::string>();
    json result = run_query(q);

    std::string payload = result.dump(2);
    return response(200, "OK", payload);
  }

  // Handle snippets requests
  std::string handle_snippets(const std::string &req) {
    size_t pos = req.find("\r\n\r\n");
    if (pos == std::string::npos) return error(400);

    std::string body = req.substr(pos + 4);
    json j = json::parse(body, nullptr, false);
    if (j.is_discarded() || !j.contains("urls")) return error(400);

    std::vector<std::string> urls = j["urls"].get<std::vector<std::string>>();
    json result = fetch_snippets(urls);

    std::string payload = result.dump(2);
    return response(200, "OK", payload);
  }

  // Constructs a full HTTP response with the given status code, message, and JSON body
  std::string response(int code, const char *msg, const std::string &body) {
    std::string hdr = "HTTP/1.1 " + std::to_string(code) + " " + msg;
    hdr += "\r\nContent-Type: application/json; charset=utf-8";
    hdr += "\r\nContent-Length: " + std::to_string(body.size());
    hdr += "\r\nConnection: close\r\n\r\n";
    std::cout << "Send response" << std::endl;
    return hdr + body;
  }

  // Error response for bad requests
  std::string error(int code) {
    return response(code, "Bad Request", "{}");
  }

public:
  // Register this plugin on startup
  SearchAPI() {
    Plugin = this;
  }

  // Route matching logic: this plugin only responds to /api/search/ and /api/snippets/
  bool MagicPath(const std::string path) override {
    return path == search_endpoint || path == snippets_endpoint;
  }

  // Main entry point for handling a request
  std::string ProcessRequest(std::string request) override {
    lock.Lock();
    std::string out;

    std::cout << "Received request" << std::endl;

    if (request.find(search_endpoint) != std::string::npos) {
      out = handle_search(request);
    } else if (request.find(snippets_endpoint) != std::string::npos) {
      out = handle_snippets(request);
    } else {
      out = error(404);
    }

    lock.Unlock();
    return out;
  }
};

// Instantiate once
SearchAPI search_api;

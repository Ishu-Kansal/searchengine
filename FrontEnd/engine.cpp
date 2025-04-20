// engine.cpp
#include <string>
#include <pthread.h>
#include <iostream>
#include "Plugin.h"
#include "Mutex.h"
#include "json.hpp" // Download from https://github.com/nlohmann/json"
#include "../src/driver.h"

using json = nlohmann::json;

json result;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Shared driver instance
Driver driver;

// Struct used to pass URL and index info to each thread
struct ThreadData {
  std::string_view url;
  int idx;
};

// Thread worker function for fetching title and snippet
void* snippet_thread_worker(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);
    SearchResult parsed = driver.get_url_and_parse(data->url);

    // Lock before modifying shared data
    pthread_mutex_lock(&result_mutex);
    result["results"][data->idx] = {
        {"url", parsed.url},
        {"title", parsed.title},
        {"snippet", parsed.snippet}
    };
    pthread_mutex_unlock(&result_mutex);

    delete data; // Clean up dynamically allocated memory
    return nullptr;
}

// Executes a query by calling the search backend (run_engine),
// then concurrently fetching the matching URLs.
// This modified version returns just the URLs.
json run_query(std::string &query) {
  std::vector<std::string_view> urls = driver.run_engine(query);

  result["results"] = json::array();
  for (int i = 0; i < urls.size(); i++) {
    result["results"].push_back({
        {"url", urls[i]},
        {"title", urls[i]},
        {"snippet", ""}
    });
  }

  return result;
}

// Executes fetching of snippets for a given list of URLs
json fetch_snippets(std::vector<std::string_view> &urls) {
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

    std::vector<std::string_view> urls = j["urls"].get<std::vector<std::string_view>>();
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

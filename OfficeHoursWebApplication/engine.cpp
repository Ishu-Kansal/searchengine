/*
 * engine.cpp
 * 
 * Implements a multithreaded search API endpoint using a plugin-based architecture.
 * The module handles HTTP requests to `/api/search/`, extracts a search query from JSON,
 * runs a search engine, fetches and parses the result pages in parallel using POSIX threads,
 * and responds with a formatted JSON payload.
 * 
 * Dependencies:
 * - nlohmann::json (https://github.com/nlohmann/json)
 * - pthread (POSIX threads)
 * - driver.h: Contains `run_engine` and `get_and_parse_url`
 * - Plugin.h, Mutex.h: Used for request handling and thread safety
 */

#include <string>
#include "Plugin.h"
#include "Mutex.h"
#include "json.hpp"   // Download from https://github.com/nlohmann/json"
#include "../src/driver.h"
#include <pthread.h>

//#include <chrono>
//#include <thread>

using json = nlohmann::json;

json result;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Struct passed to each thread, containing the URL to process and its index in the result list.
struct ThreadData {
  cstring_view url;
  int idx;
};

/**
 * @brief Worker function executed by each thread.
 * 
 * Fetches and parses the URL assigned to this thread, then writes the result into
 * the global `result` object at the appropriate index.
 * 
 * @param arg Pointer to a ThreadData struct.
 * @return void* Always returns nullptr.
 */
void* thread_worker(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);
    SearchResult parsed = get_and_parse_url(data->url);

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

/**
 * @brief Executes the main search query logic.
 * 
 * Given a search string, it calls the `run_engine` backend, then creates a thread
 * for each resulting URL to fetch and parse the page content concurrently.
 * 
 * @param query The user's search query string.
 * @return json JSON object containing an array of result objects (with URL, title, snippet).
 */
json run_query(std::string &query) {

  // call driver and return dict of results
  std::vector<cstring_view> urls = run_engine(query);

  if (urls.empty()) {
    result["results"] = json::array();
    return result;
  }

  result["results"] = json::array();

  std::vector<pthread_t> threads;

  for (int i = 0; i < urls.size(); i++) {
      pthread_t thread;
      ThreadData* data = new ThreadData{urls[i], i}; // Allocate thread-specific data

      if (pthread_create(&thread, nullptr, thread_worker, data) != 0) {
          std::cerr << "Failed to create thread for URL: " << urls[i] << std::endl;
          delete data; // Clean up if failed
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

/**
 * @brief SearchAPI class implementing PluginObject interface.
 * 
 * Registers a custom handler for the `/api/search/` path and ensures thread-safe
 * processing of search requests using an internal Mutex lock.
 */
class SearchAPI : public PluginObject {
private:
  Mutex lock;
  const std::string endpoint = "/api/search/";

  /**
   * @brief Handles an HTTP request string and returns a JSON response.
   * 
   * Parses the request body, extracts the query field, and runs the search engine.
   * 
   * @param req Full HTTP request string.
   * @return std::string HTTP response string.
   */
  std::string handle(const std::string &req) {
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

  /**
   * @brief Constructs a full HTTP response string.
   * 
   * @param code HTTP status code.
   * @param msg Status message (e.g., "OK", "Bad Request").
   * @param body Response body content.
   * @return std::string Fully formatted HTTP response.
   */
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
  SearchAPI() {
    Plugin = this;
  }

  bool MagicPath(const std::string path) override {
    return path == endpoint;
  }

  /**
   * @brief Processes an incoming request in a thread-safe manner.
   * 
   * Locks the mutex, handles the request, then unlocks it.
   * 
   * @param request The raw HTTP request string.
   * @return std::string The formatted HTTP response string.
   */
  std::string ProcessRequest(std::string request) override {
    lock.Lock();
    std::string out = handle(request);
    lock.Unlock();
    return out;
  }
};

// Instantiate once
SearchAPI search_api;

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

// Struct to pass data to each thread
struct ThreadData {
  cstring_view url;
  int idx;
};

// Thread function
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

// Forward‑declare your search routine. Implement it elsewhere.
json run_query(std::string &query) {
  //json result

  // call driver and return dict of results
  std::vector<cstring_view> urls = run_engine(query);

  if (urls.empty()) {
    result["results"] = json::array();
    return result;
  }

  result["results"] = json::array();

  // for (const auto &url : urls) {
  //   SearchResult parsed = get_and_parse_url(url);

  //   result["results"].push_back({
  //     {"url", parsed.url},
  //     {"title", parsed.title},
  //     {"snippet", parsed.snippet}
  //   });
  // }

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

  //std::this_thread::sleep_for(std::chrono::seconds(8));

  return result;
}

class SearchAPI : public PluginObject {
private:
  Mutex lock;
  const std::string endpoint = "/api/search/";

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

  std::string response(int code, const char *msg, const std::string &body) {
    std::string hdr = "HTTP/1.1 " + std::to_string(code) + " " + msg;
    hdr += "\r\nContent-Type: application/json; charset=utf-8";
    hdr += "\r\nContent-Length: " + std::to_string(body.size());
    hdr += "\r\nConnection: close\r\n\r\n";
    return hdr + body;
  }

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

  std::string ProcessRequest(std::string request) override {
    lock.Lock();
    std::string out = handle(request);
    lock.Unlock();
    return out;
  }
};

// Instantiate once
SearchAPI search_api;

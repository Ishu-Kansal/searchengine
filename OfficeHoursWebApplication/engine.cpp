#include <string>
#include "Plugin.h"
#include "Mutex.h"
#include "json.hpp"   // Download from https://github.com/nlohmann/json
// #include "../src/driver.cpp"


using json = nlohmann::json;

// Forward‑declare your search routine. Implement it elsewhere.
json run_query(const std::string &query) {
  json result;

  // // call driver and return dict of results
  // std::vector<std::string> urls = run_engine(query);

  // if (urls.empty()) {
  //   result["results"] = json::array();
  //   return result;
  // }

  // for (const auto& url : urls) {
  //   result["results"].push_back({
  //     {"url", url}
  //   });
  // }

  result["results"] = {
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 1"},
      {"url", "http://example.com/page1"},
      {"snippet", "You searched for: " + query}
    },
    {
      {"title", "Test Result 2"},
      {"url", "http://example.com/page2"},
      {"snippet", "This is another result for query: " + query}
    }
  };
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

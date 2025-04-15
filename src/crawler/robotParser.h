#pragma once
#include <sstream>
#include <string>

#include "sockets.h"

static constexpr std::string_view USER_AGENT = "SonOfAnton/1.0";

class RobotParser {
 public:
  bool isEqual(std::string_view a, std::string_view b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(),
                      [](unsigned char c1, unsigned char c2) {
                        return std::tolower(c1) == std::tolower(c2);
                      });
  }
  std::string_view trim_view(std::string_view sv) {
    // trim leading and trailing whitespace
    const auto first = sv.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string_view::npos) return {};

    const auto last = sv.find_last_not_of(" \t\n\r\f\v");
    return sv.substr(first, last - first + 1);
  }
  std::string getHostFromUrl(const std::string& url) {
    std::string::size_type pos = url.find("://");
    if (pos == std::string::npos) return "";
    pos += 3;
    std::string::size_type endPos = url.find('/', pos);
    return url.substr(pos, endPos - pos);
  }
  void parseRobotFile(std::string& robotTxt) {
    std::istringstream stream(robotTxt);
    std::string line;
    bool ourUserAgentFound = false;
    while (std::getline(stream, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();

      std::string_view line_view(line);

      line_view = trim_view(line_view);

      if (line_view.empty() || line_view[0] == '#') continue;

      size_t pos = line_view.find(':');
      if (pos == std::string_view::npos) continue;

      std::string_view field_view = line_view.substr(0, pos);
      std::string_view value_view = line_view.substr(pos + 1);

      field_view = trim_view(field_view);
      value_view = trim_view(value_view);

      if (isEqual(field_view, "User-agent")) {
        if (isEqual(value_view, USER_AGENT)) ourUserAgentFound = true;
        if (isEqual(value_view, "*")) }
    }
  }
  int handleRobotFile(const ParsedUrl& url, const std::string& url_in) {
    std::string host = getHostFromUrl(url.Path);
    if (!host.empty()) {
      // request to https://<host>/robots.txt
      std::string req =
          "GET /https://" + host + "/robots.txt" +
          " HTTP/1.1\r\n"
          "Host: " +
          std::string(url.Host) +
          "\r\n"
          "User-Agent: SonOfAnton/1.0 404FoundEngine@umich.edu (Linux)\r\n"
          "Accept: */*\r\n"
          "Accept-Encoding: identity\r\n"
          "Connection: close\r\n\r\n";

      std::string output;

      int status = runSocket(req, url_in, output);
      if (status == 404) return 0;  // No robots.txt
      parseRobotFile(output);       // parse robots.txt
    }
    return 0;
  }
};
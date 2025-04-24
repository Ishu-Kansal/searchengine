#pragma once

#include <cstdint>
#include <iostream>

constexpr int MAX_EXPECTED_LINKS = 10'000'000;
constexpr uint32_t MAX_PROCESSED = 100'000;
constexpr uint64_t MAX_QUEUE_SIZE = 10'000;
constexpr uint64_t MAX_VECTOR_SIZE = 100'000;
constexpr uint64_t NUM_RANDOM = 15'000;
constexpr uint64_t TOP_K_ELEMENTS = 7'500;
constexpr uint64_t DISPATCHER_SAVE_RATE = 25'000;
constexpr double MAX_FALSE_POSITIVE_RATE = 1e-2;

constexpr uint64_t NUM_CHUNKS = 10;  // start small

using header_t = uint64_t;
constexpr uint64_t GET_COMMAND = 1;

constexpr int GET_PORT = 1234;
constexpr int ADD_PORT = 2345;

constexpr const char *const SERVER_PORT_STR = "8080";

constexpr const char *filterName = "dispatcher_filter.bin";
constexpr const char *queueName = "dispatcher_queue.bin";
constexpr const char *statsName = "dispatcher_stats.bin";

struct UrlHostPair {
  std::string url;
  std::string host;
  uint64_t rank;

  friend std::ostream &operator<<(std::ostream &os, const UrlHostPair &uhp) {
    return os << ' ' << uhp.url << ' ' << uhp.host << ' ' << uhp.rank << '\n';
  }

  friend std::istream &operator>>(std::istream &is, UrlHostPair &uhp) {
    return is >> uhp.url >> uhp.host >> uhp.rank;
  }
};

struct UrlRankPair {
  std::string url;
  uint64_t rank;
};

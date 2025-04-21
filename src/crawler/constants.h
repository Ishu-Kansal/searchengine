#pragma once

#include <cstdint>

constexpr int MAX_EXPECTED_LINKS = 1'000'000;
constexpr uint32_t MAX_PROCESSED = 10;
// constexpr uint64_t MAX_QUEUE_SIZE = 100'000;
constexpr uint64_t MAX_VECTOR_SIZE = 100'000;
constexpr uint64_t NUM_RANDOM = 10'000;
constexpr uint64_t TOP_K_ELEMENTS = 7'500;
constexpr double MAX_FALSE_POSITIVE_RATE = 1e-2;

using header_t = uint64_t;
constexpr uint64_t GET_COMMAND = 1;
constexpr uint64_t SAVE_COMMAND = 2;
constexpr uint64_t SAVE_SUCCESS = 1;

constexpr int SERVER_PORT = 8080;
constexpr const char *const SERVER_PORT_STR = "8080";

constexpr const char *filterName = "dispatcher_filter.bin";
constexpr const char *queueName = "dispatcher_queue.bin";
constexpr const char *statsName = "dispatcher_stats.bin";
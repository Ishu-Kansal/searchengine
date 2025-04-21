#pragma once

#include <cstdint>

constexpr int MAX_EXPECTED_LINKS = 10'000'000;
constexpr uint32_t MAX_PROCESSED = 100'000;
// constexpr uint64_t MAX_QUEUE_SIZE = 100'000;
constexpr uint64_t MAX_VECTOR_SIZE = 100'000;
constexpr uint64_t NUM_RANDOM = 15'000;
constexpr uint64_t TOP_K_ELEMENTS = 7'500;
constexpr uint64_t DISPATCHER_SAVE_RATE = 25'000;
constexpr double MAX_FALSE_POSITIVE_RATE = 1e-2;

using header_t = uint64_t;
constexpr uint64_t GET_COMMAND = 1;

constexpr int SERVER_PORT = 8080;
constexpr const char *const SERVER_PORT_STR = "8080";

constexpr const char *filterName = "dispatcher_filter.bin";
constexpr const char *queueName = "dispatcher_queue.bin";
constexpr const char *statsName = "dispatcher_stats.bin";

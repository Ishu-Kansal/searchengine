#include <cstdint>

constexpr int MAX_EXPECTED_LINKS = 1'000'000;
constexpr uint64_t MAX_QUEUE_SIZE = 100'000;
constexpr uint64_t MAX_VECTOR_SIZE = 20'000;
constexpr uint64_t NUM_RANDOM = 10'000;
constexpr uint64_t TOP_K_ELEMENTS = 7'500;
constexpr double MAX_FALSE_POSITIVE_RATE = 1e-3;

constexpr int SERVER_PORT = 8080;
constexpr const char *const SERVER_PORT_STR = "8080";
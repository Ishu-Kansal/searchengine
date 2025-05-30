#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <pthread.h>

#include <cassert>
#include <cstdint>
#include <queue>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "../../../BloomFilterStarterFiles/BloomFilter.h"
#include "../../../utils/pthread_lock_guard.h"
#include "../info.grpc.pb.h"

using cloudcrawler::AddRequest;
using cloudcrawler::Dispatcher;
using cloudcrawler::Empty;
using cloudcrawler::GetResponse;
using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerUnaryReactor;
using grpc::ServerWriter;
using grpc::Status;

constexpr uint64_t MAX_EXPECTED_LINKS = 100'000'000;
constexpr uint64_t MAX_QUEUE_SIZE = 100'000;
constexpr uint64_t MAX_VECTOR_SIZE = 20'000;
constexpr uint64_t NUM_RANDOM = 10'000;
constexpr uint64_t TOP_K_ELEMENTS = 7'500;
constexpr double MAX_FALSE_POSITIVE_RATE = 1e3;

class DispatcherImpl final : public Dispatcher::CallbackService {
 public:
  explicit DispatcherImpl() : bf(MAX_EXPECTED_LINKS, MAX_FALSE_POSITIVE_RATE) {
    int res = pthread_mutex_init(&queue_lock, NULL);
    assert(res == 0);
  }

  ServerUnaryReactor *GetUrl(CallbackServerContext *context,
                             const Empty *request,
                             GetResponse *response) override {
    auto *reactor = context->DefaultReactor();
    pthread_lock_guard{queue_lock};
    ++num_processed;
    const int res =
        (num_processed < MAX_EXPECTED_LINKS) ? get_next_url(response) : -1;
    if (res == 0)
      reactor->Finish(Status::OK);
    else
      reactor->Finish(Status::CANCELLED);
    return reactor;
  }

  ServerUnaryReactor *AddUrls(CallbackServerContext *context,
                              const AddRequest *request,
                              Empty *response) override {
    auto *reactor = context->DefaultReactor();
    pthread_lock_guard{queue_lock};
    if (links_vector.size() < MAX_VECTOR_SIZE)
      links_vector.emplace_back(request->url(), request->rank());
    reactor->Finish(Status::OK);
    return reactor;
  }

 private:
  Bloomfilter bf;

  std::mt19937 mt{std::random_device{}()};
  std::queue<std::string> explore_queue{};
  std::vector<std::pair<std::string, uint32_t>> links_vector{};

  uint64_t num_processed{};

  pthread_mutex_t queue_lock{};

 private:
  int get_next_url(GetResponse *response) {
    const size_t links_vector_size = links_vector.size();

    if (explore_queue.empty() && links_vector_size > MAX_VECTOR_SIZE) {
      fill_queue();
    }

    if (!explore_queue.empty()) {
      response->set_url(std::move(explore_queue.front()));
      explore_queue.pop();
    } else if (!links_vector.empty()) {
      // std::cout << "Explore queue empty, use last link from vector"
      response->set_url(std::move(links_vector.back().first));
      links_vector.pop_back();
    } else {
      return -1;
    }
    return 0;
  }

  int partition(int left, int right, int pivot_index) {
    int pivot_rank = links_vector[pivot_index].second;

    // Move the pivot to the end
    std::swap(links_vector[pivot_index], links_vector[right]);

    // Move all less ranked elements to the left
    int store_index = left;
    for (int i = left; i < right; i++) {
      if (links_vector[i].second < pivot_rank) {
        std::swap(links_vector[store_index], links_vector[i]);
        store_index += 1;
      }
    }

    // Move the pivot to its final place
    std::swap(links_vector[right], links_vector[store_index]);

    return store_index;
  }

  void quickselect(int left, int right, int k) {
    if (left >= right) {
      return;
    }

    std::uniform_int_distribution<> gen(left, right - 1);
    int pivot_index = gen(mt);

    // Find the pivot position in a sorted list
    pivot_index = partition(left, right, pivot_index);

    // If the pivot is in its final sorted position
    if (k == pivot_index) {
      return;
    } else if (k < pivot_index) {
      // go left
      quickselect(left, pivot_index - 1, k);
    } else {
      // go right
      quickselect(pivot_index + 1, right, k);
    }
  }

  void fill_queue() {
    static_assert(NUM_RANDOM < MAX_VECTOR_SIZE);
    // std::cout << "Enter fill_queue()" << std::endl;
    uint32_t links_vector_size = links_vector.size();

    if (explore_queue.empty() && links_vector_size > MAX_VECTOR_SIZE) {
      // std::cout << "Here1" << std::endl;
      // Establish range for uniform random num gen
      // Range is from 0 to the last element in the vector
      std::uniform_int_distribution<> gen{0, int(links_vector_size - 1)};
      // std::cout << "Here2" << std::endl;

      // Generates N random elements and moves them to the end
      for (size_t t = links_vector_size - NUM_RANDOM; t < links_vector_size;
           t++) {
        std::swap(links_vector[gen(mt)], links_vector[t]);
      }
      // std::cout << "Here3" << std::endl;

      // Sorts the last N elements of the vector
      quickselect(links_vector_size - NUM_RANDOM, links_vector_size - 1,
                  NUM_RANDOM - TOP_K_ELEMENTS);

      // std::cout << "Here4" << std::endl;

      // Takes last K from vector and adds its to queue
      for (size_t i = 0; i < TOP_K_ELEMENTS; ++i) {
        explore_queue.push(std::move(links_vector.back().first));
        links_vector.pop_back();
      }
    }
    // std::cout << "Exit fill_queue()" << std::endl;
  }
};
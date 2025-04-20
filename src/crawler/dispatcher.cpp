#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "../../BloomFilterStarterFiles/BloomFilter.h"
#include "../../utils/cstring_view.h"
#include "../../utils/pthread_lock_guard.h"
#include "../../utils/socket_wrapper.h"
#include "../../utils/utf_encoding.h"
#include "constants.h"

pthread_mutex_t queue_lock{};
Bloomfilter bf(1, 1);

std::mt19937 mt{std::random_device{}()};
std::queue<std::string> explore_queue{};
std::vector<std::pair<std::string, uint32_t>> links_vector{};

uint64_t num_processed{};

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

std::string get_next_url() {
  const size_t links_vector_size = links_vector.size();

  if (explore_queue.empty() && links_vector_size > MAX_VECTOR_SIZE) {
    fill_queue();
  }

  if (!explore_queue.empty()) {
    std::string res = std::move(explore_queue.front());
    explore_queue.pop();
    return res;
  } else if (!links_vector.empty()) {
    // std::cout << "Explore queue empty, use last link from vector"
    std::uniform_int_distribution<> gen{0, int(links_vector_size - 1)};
    std::swap(links_vector[gen(mt)], links_vector.back());
    std::string res = std::move(links_vector.back().first);
    links_vector.pop_back();
    return res;
  } else {
    return "";
  }
}

void get_handler(int fd) {
  pthread_lock_guard _{queue_lock};
  const std::string next = get_next_url();
  const size_t header = next.size();
  send(fd, &header, sizeof(header), 0);
  send(fd, next.data(), next.size(), 0);
}

void save_handler(int sock) {
  pthread_lock_guard _{queue_lock};
  static const char *filterName = "dispatcher_filter.bin";
  static const char *queueName = "dispatcher_queue.bin";
  remove(filterName);
  remove(queueName);
  int fd = open(filterName, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    perror("");
  }
  bf.writeBFtoFile(fd);

  std::ofstream outputFile{queueName};

  int ctr = 0;
  std::queue<std::string> temp_queue{};
  while (ctr < 100 && !explore_queue.empty()) {
    outputFile << explore_queue.front() << '\n';
    temp_queue.push(std::move(explore_queue.front()));
    ++ctr;
    explore_queue.pop();
  }
  while (!temp_queue.empty()) {
    explore_queue.push(std::move(temp_queue.front()));
    temp_queue.pop();
  }
  int right_ptr = int(links_vector.size()) - 1;
  while (ctr < 100 && right_ptr >= 0) {
    outputFile << links_vector[right_ptr].first << '\n';
    --right_ptr;
    ++ctr;
  }
  outputFile.flush();
  send(sock, &SAVE_SUCCESS, sizeof(SAVE_SUCCESS), 0);
}

void add_handler(int fd, uint64_t size) {
  {
    pthread_lock_guard _{queue_lock};
    if (size <= sizeof(uint64_t) || links_vector.size() > MAX_VECTOR_SIZE)
      return;
  }
  std::string req(size, 0);
  ssize_t bytes = 0;
  uint64_t rank = 0;
  if ((bytes = recv(fd, req.data(), req.size(), MSG_WAITALL)) <= 0) {
    return;
  }
  memcpy(&rank, req.data(), sizeof(rank));
  auto url = req.substr(sizeof(rank));

  pthread_lock_guard _{queue_lock};
  if (links_vector.size() < MAX_VECTOR_SIZE && !bf.contains(url)) {
    bf.insert(url);
    links_vector.emplace_back(std::move(url), rank);
  }
}

void *handler(void *fd) {
  /*static const cstring_view GET_COMMAND{"GET"};
  static const cstring_view ADD_COMMAND{"ADD"};
  static const cstring_view SAVE_COMMAND{"SAVE"};
  static const cstring_view SAVE_CONFIRMED{"CONFIRMED"};*/
  int *t = reinterpret_cast<int *>(fd);
  const int sock = *t;
  std::unique_ptr<int> _{t};
  SocketWrapper sock_{sock};
  header_t val;
  if (recv(sock, &val, sizeof(val), 0) <= 0) {
    return NULL;
  }
  if (val == GET_COMMAND) {
    get_handler(sock);
  } else if (val == SAVE_COMMAND) {
    save_handler(sock);
  } else {
    add_handler(sock, val);
  }
  return NULL;
}

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);
  int res = pthread_mutex_init(&queue_lock, NULL);
  if (res != 0) return 1;
  bf = Bloomfilter(MAX_EXPECTED_LINKS, MAX_FALSE_POSITIVE_RATE);
  std::ifstream infile("seed_list.txt");
  if (!infile.is_open()) {
    std::cerr << "Failed to open seed list" << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string line;
  std::uniform_int_distribution<> pushDist(1, 25);
  while (std::getline(infile, line)) {
    if (line.empty() || pushDist(mt) != 1) continue;
    explore_queue.push(line);
    bf.insert(line);
  }
  infile.close();
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(sock != -1);

  int opt = 1;
  int opt_res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  assert(opt_res != -1);

  sockaddr_in addr{};  // initializes with zeroes
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(SERVER_PORT);

  int bind_res = bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
  assert(bind_res != -1);

  unsigned int len = sizeof(addr);

  int listen_res = listen(sock, 100);
  assert(listen_res != -1);

  memset(&addr, 0, sizeof(addr));

  while (true) {
    int newfd = accept(sock, reinterpret_cast<sockaddr *>(&addr),
                       reinterpret_cast<socklen_t *>(&len));
    if (newfd == -1) continue;
    pthread_t temp;
    pthread_create(&temp, NULL, handler, new int{newfd});
    pthread_detach(temp);
  }
}
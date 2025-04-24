#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
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
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../BloomFilterStarterFiles/BloomFilter.h"
#include "../../utils/cstring_view.h"
#include "../../utils/pthread_lock_guard.h"
#include "../../utils/socket_wrapper.h"
#include "../../utils/utf_encoding.h"
#include "constants.h"

Bloomfilter bf(1, 1);

std::mt19937 mt{std::random_device{}()};
std::queue<UrlHostPair> explore_queue{};
std::vector<UrlHostPair> links_vector{};

uint64_t num_processed{};

pthread_mutex_t queue_lock{};
std::unordered_map<std::string, uint32_t> ref_cts{};

std::string get_host(const std::string &url_string) {
  std::string host;

  size_t scheme_end = url_string.find("://");
  if (scheme_end == std::string::npos) {
    return "EMPTY";
  }
  size_t authority_start = scheme_end + 3;

  size_t authority_end = url_string.find_first_of("/?#", authority_start);

  if (authority_end == std::string::npos) {
    return url_string.substr(authority_start);
  } else {
    return url_string.substr(authority_start, authority_end - authority_start);
  }
}

int partition(int left, int right, int pivot_index) {
  int pivot_rank = links_vector[pivot_index].rank;

  // Move the pivot to the end
  std::swap(links_vector[pivot_index], links_vector[right]);

  // Move all less ranked elements to the left
  int store_index = left;
  for (int i = left; i < right; i++) {
    if (links_vector[i].rank < pivot_rank) {
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

  if (explore_queue.empty() && links_vector_size + 100 > MAX_VECTOR_SIZE) {
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
      explore_queue.push(std::move(links_vector.back()));
      links_vector.pop_back();
    }
  }
  // std::cout << "Exit fill_queue()" << std::endl;
}

UrlHostPair get_next_url() {
  const size_t links_vector_size = links_vector.size();

  if (explore_queue.empty() && links_vector_size + 100 > MAX_VECTOR_SIZE) {
    fill_queue();
  }

  if (!explore_queue.empty()) {
    UrlHostPair res = std::move(explore_queue.front());
    explore_queue.pop();
    return res;
  } else if (!links_vector.empty()) {
    // std::cout << "Explore queue empty, use last link from vector"
    std::uniform_int_distribution<> gen{0, int(links_vector_size - 1)};
    std::swap(links_vector[gen(mt)], links_vector.back());
    UrlHostPair res = std::move(links_vector.back());
    links_vector.pop_back();
    return res;
  } else {
    return {};
  }
}

void saver() {
  remove(filterName);
  remove(queueName);
  std::cout << "Writing filter...\n";
  bf.writeBFtoFile(filterName);
  std::cout << "Finished writing filter...\n";

  std::ofstream outputFile{queueName,
                           std::ofstream::out | std::ios_base::trunc};

  int ctr = 0;
  std::queue<UrlHostPair> temp_queue{};
  std::cout << "Writing queue...\n";
  while (ctr < 1000 && !explore_queue.empty()) {
    outputFile << explore_queue.front();
    temp_queue.push(std::move(explore_queue.front()));
    ++ctr;
    explore_queue.pop();
  }
  while (!temp_queue.empty()) {
    explore_queue.push(std::move(temp_queue.front()));
    temp_queue.pop();
  }
  int right_ptr = int(links_vector.size()) - 1;
  while (right_ptr >= 0 && ctr < 1000) {
    outputFile << links_vector[right_ptr];
    --right_ptr;
    ++ctr;
  }
  outputFile.flush();
  std::cout << "Finished writing queue...\n";
  std::ofstream statsFile{statsName, std::ofstream::out | std::ios_base::trunc};
  statsFile << num_processed;
  statsFile.flush();
}

void print_dispatcher() {
  std::cout << "Dispatcher size: " << links_vector.size() + explore_queue.size()
            << '\n';
}

void get_handler(int fd) {
  UrlHostPair next;
  {
    pthread_lock_guard guard{queue_lock};
    next = get_next_url();
    ref_cts[next.host]--;

    num_processed++;
    if (num_processed % 1000 == 0) {
      std::cout << num_processed << std::endl;
      print_dispatcher();
    }
    if (num_processed % DISPATCHER_SAVE_RATE == 0) saver();
  }
  const size_t header = next.url.size();
  send(fd, &header, sizeof(header), 0);
  send(fd, &next.rank, sizeof(next.rank), 0);
  send(fd, next.url.data(), next.url.size(), 0);
}

/*void add_handler(int fd, uint64_t size) {
  if (size <= sizeof(uint64_t) || links_vector.size() > MAX_VECTOR_SIZE) return;
  uint64_t rank = 0;
  ssize_t bytes = 0;
  std::string url(size - sizeof(rank), 0);
  if (url.size() == 0) return;
  if ((bytes = recv(fd, &rank, sizeof(rank), MSG_WAITALL)) <= 0) return;
  if ((bytes = recv(fd, url.data(), url.size(), MSG_WAITALL)) <= 0) return;

  if (!bf.contains(url)) {
    bf.insert(url);
    links_vector.emplace_back(std::move(url), rank);
  }
}

void *handler(void *fd) {
  int sock = (uint64_t)(fd);
  SocketWrapper sock_{sock};

  header_t val;

  if (recv(sock, &val, sizeof(val), 0) <= 0) {
    return NULL;
  }
  if (val == GET_COMMAND) {
    get_handler(sock);
  } else {
    add_handler(sock, val);
  }
  return NULL;
}*/

void init_dispatcher() {
  std::ifstream if1{queueName}, if2{statsName};
  if (if1.good()) {
    bf = Bloomfilter(filterName);
    UrlHostPair url;
    while (if1 >> url) {
      explore_queue.emplace(url);
      ref_cts[url.host]++;
    }
    if2 >> num_processed;
  } else {
    bf = Bloomfilter(MAX_EXPECTED_LINKS, MAX_FALSE_POSITIVE_RATE);
    std::ifstream infile("seed_list.txt");
    if (!infile.is_open()) {
      std::cerr << "Failed to open seed list" << std::endl;
      exit(EXIT_FAILURE);
    }
    std::string line;
    while (std::getline(infile, line)) {
      auto host = get_host(line);
      ref_cts[host]++;
      explore_queue.emplace(line, std::move(host), 10);
      bf.insert(line);
    }
    infile.close();
  }
}

void *getter(void *arg) {
  int fd = (uint64_t)(arg);
  char c;
  while (recv(fd, &c, sizeof(c), 0) != 0) {
    get_handler(fd);
  }
  close(fd);
  return NULL;
}

void *get_requests(void *) {
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(sock != -1);

  int opt = 1;
  int opt_res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  assert(opt_res != -1);

  int flag = 1;
  opt_res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
  assert(opt_res != -1);

  sockaddr_in addr{};  // initializes with zeroes
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(GET_PORT);

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
    pthread_t thread;
    pthread_create(&thread, NULL, getter, (void *)(uint64_t)(newfd));
    pthread_detach(thread);
  }
}

void *adder(void *arg) {
  int fd = (uint64_t)(arg);
  size_t header;
  uint64_t rank;
  while (recv(fd, &header, sizeof(header), MSG_WAITALL) != 0) {
    uint64_t rank{};
    std::string url(header, 0);
    if (recv(fd, &rank, sizeof(rank), MSG_WAITALL) == 0) break;
    if (recv(fd, url.data(), url.size(), MSG_WAITALL) == 0) break;
    std::string host = get_host(url);
    UrlHostPair p;
    p.rank = rank;
    p.host = std::move(host);
    p.url = std::move(url);
    pthread_lock_guard guard{queue_lock};
    if (links_vector.size() < MAX_VECTOR_SIZE && !bf.contains(p.url) &&
        ref_cts[p.host] < 10'000) {
      bf.insert(p.url);
      ref_cts[p.host]++;
      links_vector.emplace_back(std::move(p));
    }
  }
  close(fd);
  return NULL;
}

void *add_requests(void *) {
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(sock != -1);

  int opt = 1;
  int opt_res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  assert(opt_res != -1);

  int flag = 1;
  opt_res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
  assert(opt_res != -1);

  sockaddr_in addr{};  // initializes with zeroes
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(ADD_PORT);

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
    pthread_t thread;
    pthread_create(&thread, NULL, adder, (void *)(uint64_t)(newfd));
    pthread_detach(thread);
  }
}

int main(int argc, char **argv) {
  std::cout << "STARTING DISPATCHER...\n";

  signal(SIGPIPE, SIG_IGN);

  init_dispatcher();

  pthread_mutex_init(&queue_lock, NULL);

  pthread_t getter, adder;
  pthread_create(&getter, NULL, get_requests, NULL);
  pthread_create(&adder, NULL, add_requests, NULL);
  pthread_join(getter, NULL);
  pthread_join(adder, NULL);
}
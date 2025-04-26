#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "../../BloomFilterStarterFiles/BloomFilter.h"
#include "../../HtmlParser/HtmlParser.h"
#include "../../utils/cstring_view.h"
#include "../../utils/pthread_lock_guard.h"
#include "../../utils/socket_wrapper.h"
#include "../inverted_index/Index.h"
#include "../inverted_index/IndexFile.h"
#include "../ranker/rank.h"
#include "constants.h"
#include "sockets.h"

// #define NO_INDEX

struct ChunkBlock {
  IndexChunk chunk;
  pthread_mutex_t lock;
  sem_t* sem;
};
// Get error when i try to use cstring_view
static constexpr std::string_view APOS_ENTITY = "&#039;";
static constexpr std::string_view APOS_ENTITY2 = "&apos;";
static constexpr std::string_view HTML_ENTITY = "&#";

static constexpr std::string_view UNWANTED_NBSP = "&nbsp";
static constexpr std::string_view UNWANTED_LRM = "&lrm";
static constexpr std::string_view UNWANTED_RLM = "&rlm";
static constexpr size_t MAX_WORD_LENGTH = 50;

const static int NUM_THREADS = 384;  // start small

uint32_t STATIC_RANK = 0;  // temp global variable

std::atomic<uint32_t> num_created{};

std::queue<std::string> explore_queue{};
std::vector<std::pair<std::string, uint32_t>> links_vector;
IndexChunk chunks[NUM_CHUNKS];
pthread_mutex_t chunk_locks[NUM_CHUNKS];
sem_t* sems[NUM_THREADS];

pthread_mutex_t queue_lock{};
pthread_mutex_t chunk_lock{};
pthread_mutex_t cout_lock{};
sem_t* queue_sem;
// uint32_t num_processed{};
std::mt19937 mt{std::random_device{}()};

std::atomic<int> ctr{}, num_processed{};
std::atomic<int> get_requests{};

pthread_mutex_t getter_lock{};
sem_t* getter_request_sem{};
sem_t* getter_response_sem{};

pthread_mutex_t adder_lock{};
sem_t* adder_request_sem{};

std::vector<UrlRankPair> getterQueue{};
std::vector<std::pair<std::string, uint64_t>> adderQueue{};

std::vector<pthread_t> add_to_index_threads;
pthread_mutex_t add_to_index_threads_lock;

std::string dispatcher_address{};

int adder_socket;

void* url_getter(void*) {
  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(GET_PORT);  // Port number
  address.sin_addr.s_addr = inet_addr(dispatcher_address.c_str());  // Server IP

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(sock != -1);

  int flag = 1;
  int res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
  assert(res != -1);

  res = connect(sock, (struct sockaddr*)&address, sizeof(address));
  assert(res != -1);

  char req = 1;
  size_t header{};
  uint64_t dist{};

  while (true) {
    sem_wait(getter_request_sem);
    send(sock, &req, sizeof(req), MSG_NOSIGNAL);
    recv(sock, &header, sizeof(header), MSG_WAITALL);
    recv(sock, &dist, sizeof(dist), MSG_WAITALL);
    std::string next_url(header, 0);
    recv(sock, next_url.data(), next_url.size(), MSG_WAITALL);
    ++get_requests;
    // std::cout << "Number of get requests: " << get_requests << " and url "
    //         << next_url << '\n';
    {
      pthread_lock_guard guard{getter_lock};
      getterQueue.emplace_back(std::move(next_url), dist);
      sem_post(getter_response_sem);
    }
  }
}

UrlRankPair get_string() {
  sem_post(getter_request_sem);
  sem_wait(getter_response_sem);
  pthread_lock_guard guard{getter_lock};
  UrlRankPair result = std::move(getterQueue.back());
  getterQueue.pop_back();
  return result;
}

void* url_adder(void*) {
  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(ADD_PORT);  // Port number
  address.sin_addr.s_addr = inet_addr(dispatcher_address.c_str());  // Server IP

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(sock != -1);

  int flag = 1;
  int res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
  assert(res != -1);

  res = connect(sock, (struct sockaddr*)&address, sizeof(address));
  assert(res != -1);

  bool req = 1;
  size_t header{};
  std::string next{};
  uint64_t rank{};

  while (true) {
    // for (int i = 0; i < 10'000; ++i) {
    sem_wait(adder_request_sem);
    {
      if (adderQueue.empty()) {
        pthread_lock_guard cout_guard{cout_lock};
        std::cerr << "WARNING: url_adder woke but adderQueue is empty! Skipping." << std::endl;
        continue;
    }
      pthread_lock_guard guard{adder_lock};
      std::tie(next, rank) = std::move(adderQueue.back());
      adderQueue.pop_back();
      size_t header = next.size();
      send(sock, &header, sizeof(header), MSG_NOSIGNAL);
      send(sock, &rank, sizeof(rank), MSG_NOSIGNAL);
      send(sock, next.data(), next.size(), MSG_NOSIGNAL);
    }
    // }
    // sleep(1);
  }
}

void add_url(std::string&& url, uint64_t rank) {
  pthread_lock_guard guard{adder_lock};
  if (adderQueue.size() > MAX_QUEUE_SIZE || url.empty()) return;
  adderQueue.emplace_back(std::move(url), rank);
  sem_post(adder_request_sem);
}

bool isEnglish(const std::string& s) {
  for (const auto& ch : s) {
    if (static_cast<unsigned char>(ch) > 127) {
      return false;
    }
  }
  return true;
}
void cleanString(std::string& s) {
  // Gets rid of both &#039; and &apos;
  size_t pos = 0;
  while ((pos = s.find(APOS_ENTITY, pos)) != std::string::npos ||
         (pos = s.find(APOS_ENTITY2, pos)) != std::string::npos) {
    s.erase(pos, (pos == s.find(APOS_ENTITY, pos)) ? APOS_ENTITY.length()
                                                   : APOS_ENTITY2.length());
  }
  if (s.size() > MAX_WORD_LENGTH) {
    s.clear();
    return;
  }
  // Gets rid of strings containing HTML entities
  if (s.find(HTML_ENTITY) != std::string::npos ||
      s.find(UNWANTED_NBSP) != std::string::npos ||
      s.find(UNWANTED_LRM) != std::string::npos ||
      s.find(UNWANTED_RLM) != std::string::npos) {
    s.clear();
    return;
  }
  // Gets rid of non english words
  /*
    if (!isEnglish(s)) {
    s.clear();
    return;
  }
  */

  // Gets rid of '
  s.erase(std::remove(s.begin(), s.end(), '\''), s.end());

  // Get rid of start and end non punctuation
  auto start = std::find_if(s.begin(), s.end(), ::isalnum);

  if (start == s.end()) {
    s.clear();
    return;
  }

  auto end = std::find_if(s.rbegin(), s.rend(), ::isalnum).base();

  s.erase(end, s.end());
  s.erase(s.begin(), start);
}

std::vector<std::string> splitHyphenWords(const std::string& word) {
  std::vector<std::string> parts;
  if (word.find('-') != std::string::npos) {
    size_t start = 0, end = 0;
    while ((end = word.find('-', start)) != std::string::npos) {
      std::string token = word.substr(start, end - start);
      if (!token.empty()) parts.push_back(token);
      start = end + 1;
    }
    std::string token = word.substr(start);
    if (!token.empty()) parts.push_back(token);
  } else {
    parts.push_back(word);
  }
  return parts;
}

std::string getHostFromUrl(const std::string& url) {
  // std::regex urlRe("^.*://([^/?:]+)/?.*$");
  // return std::regex_replace(url, urlRe, "$1");

  std::string::size_type pos = url.find("://");
  if (pos == std::string::npos) return "";
  pos += 3;
  std::string::size_type endPos = url.find('/', pos);
  return url.substr(pos, endPos - pos);
}

struct ThreadArgs {
  std::string url;
  std::string html;
  int status;
};

void* getHTML_wrapper(void* arg) {
  ThreadArgs* args = (ThreadArgs*)arg;

  args->status = getHTML(args->url, args->html);

  return nullptr;
}

bool check_url(cstring_view url) {
  // If link does not begin with 'http', ignore it
  if (url.size() < 4 || !url.starts_with(cstring_view{"http", 4UZ})) {
    return false;
  }

  if (url.find(cstring_view{"porn"}) != cstring_view::npos) return false;

  if (url.size() > 250) {
    return false;
  }

  return true;
}

struct Args {
  HtmlParser parser;
  std::string url;
  int static_rank;
  unsigned int thread_id;
};

void* add_to_index(void* addr) {
  --num_created;
  Args* arg = reinterpret_cast<Args*>(addr);
  ++ctr;
#ifdef NO_INDEX
  delete arg;
  return NULL;
#endif

  auto idx = arg->thread_id % NUM_CHUNKS;
  auto& chunk = chunks[idx];

  const uint16_t urlLength = arg->url.size();

  if (arg->url.find("wikipedia") == std::string::npos) {
    for (char& c : arg->url) c = tolower(static_cast<unsigned char>(c));
  }

  pthread_lock_guard guard{chunk_locks[idx]};

  chunk.add_url(arg->url, arg->static_rank);

  for (auto& word : arg->parser.titleWords) {
    cleanString(word);
    if (word.empty()) continue;
    auto parts = splitHyphenWords(word);
    for (auto& part : parts) {
      std::transform(part.begin(), part.end(), part.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      // std::cout << part << '\n';
      chunk.add_word(part, true);
    }
  }

  for (auto& word : arg->parser.words) {
    cleanString(word);
    if (word.empty()) continue;
    auto parts = splitHyphenWords(word);
    for (auto& part : parts) {
      std::transform(part.begin(), part.end(), part.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      // std::cout << part << '\n';
      chunk.add_word(part, false);
    }
  }

  chunk.add_enddoc();
  sem_post(sems[arg->thread_id]);
  delete arg;
  return NULL;
}

void* runner(void* arg) {
  // const static thread_local pid_t thread_id = syscall(SYS_gettid);
  const unsigned int thread_id = (uint64_t)(arg);

  while (num_processed < MAX_PROCESSED) {
    // Get the next url to be processed
    UrlRankPair cur = get_string();
    std::string& url = cur.url;

    if (url.empty()) {
      // sleep(1);
      continue;
    }

    ThreadArgs args = {url, "", -1};

    // Start getHTML in a new thread
    getHTML_wrapper(&args);
    /*pthread_create(&thread, nullptr, getHTML_wrapper, &args);
    pthread_join(thread, nullptr);*/

    // Continue if html code was not retrieved
    if (args.status != 0) {
      static const struct timespec time{.tv_sec = 0, .tv_nsec = 500'000'000};
      nanosleep(&time, NULL);
      // std::cout << "Could not retrieve HTML, status: " << args.status << "
      // for " << url << std::endl; perror("");
      continue;
    }
    // Parse the html code
    std::string& html = args.html;
    HtmlParser parser(html.data(), html.size());

    if (!parser.isEnglish) continue;

    int static_rank = get_static_rank(cstring_view{url}, parser);
    // Process links found by the parser
    {
      auto it = num_processed++;
      if (it % 1000 == 0) std::cout << it << std::endl;
    }

    for (auto& link : parser.links) {
      std::string next_url = std::move(link.URL);

      // Ignore links that begin with '#' or '?'
      if (next_url[0] == '#' || next_url[0] == '?') {
        continue;
      }

      // If link starts with '/', add the domain to the beginning
      // of it
      if (next_url[0] == '/') {
        next_url = url.substr(0, 8) + getHostFromUrl(url) + next_url;
      }

      if (!check_url(next_url)) {
        continue;
      }

      uint64_t rank =
          std::max(std::min(cur.rank, uint64_t{2}), uint64_t{5}) * 2;
      rank += static_rank;

      add_url(std::move(next_url), static_rank);
    }
    // --------------------------------------------------
    sem_wait(sems[thread_id]);
    /*
      ++num_created;
    pthread_t t;
    pthread_create(
        &t, NULL, add_to_index,
        new Args{std::move(parser), std::move(url), static_rank, thread_id});
    pthread_detach(t);
    */
   pthread_t t;
   Args* index_args = new Args{std::move(parser), std::move(url), static_rank, thread_id};
   int create_res = pthread_create(&t, NULL, add_to_index, index_args);

   if (create_res == 0) 
   {
       {
           pthread_lock_guard guard{add_to_index_threads_lock};
           add_to_index_threads.push_back(t);
       }
   } 
   else 
   {
       errno = create_res; 
       perror("runner: Failed to create add_to_index thread");
       delete index_args; 
       sem_post(sems[thread_id]);
   }
  }
  pthread_lock_guard guard{cout_lock};
  return NULL;
}

void* metric_collector(void*) {
  while (true) {
    ctr = 0;
    sleep(10);
    std::cout << "Num processed over last 10 seconds: " << ctr << '\n';
  }
}

int main(int argc, char** argv) {
  signal(SIGPIPE, SIG_IGN);

  if (argc != 3) {
    std::cout << "Usage: ./crawler [id] [server ip]";
    exit(1);
  }

  int id = atoi(argv[1]);
  dispatcher_address = argv[2];

  std::vector<std::string> sem_names{};
  for (int i = 0; i < NUM_THREADS; ++i) {
    sem_names.push_back("/sem_" + std::to_string(id) + "_" + std::to_string(i));
    sem_unlink(sem_names[i].data());
  }

  for (int i = 0; i < NUM_CHUNKS; ++i)
    pthread_mutex_init(chunk_locks + i, NULL);

  /*for (const auto& url : seed_urls) {
    explore_queue.push(url);
    bf.insert(url);
  }*/
  sem_unlink("/crawler_sem");
  queue_sem = sem_open("/crawler_sem", O_CREAT, 0666, explore_queue.size());
  if (queue_sem == SEM_FAILED) exit(EXIT_FAILURE);

  pthread_mutex_init(&queue_lock, NULL);
  pthread_mutex_init(&chunk_lock, NULL);
  pthread_mutex_init(&cout_lock, NULL);

  pthread_t ctr_thread;
  pthread_create(&ctr_thread, NULL, metric_collector, NULL);
  pthread_detach(ctr_thread);

  assert(!pthread_mutex_init(&getter_lock, NULL));
  assert(!pthread_mutex_init(&adder_lock, NULL));

  // sem_unlink("/getter_request_sem");
  // sem_unlink("/getter_response_sem");
  // sem_unlink("/adder_request_sem");

  // getter_request_sem = sem_open("/getter_request_sem", O_CREAT, 0666, 0);
  // getter_response_sem = sem_open("/getter_response_sem", O_CREAT, 0666, 0);
  // adder_request_sem = sem_open("/adder_request_sem", O_CREAT, 0666, 0);

  std::string getter_req_sem_name = "/getter_request_sem_" + std::to_string(id);
  std::string getter_res_sem_name = "/getter_response_sem_" + std::to_string(id);
  std::string adder_req_sem_name = "/adder_request_sem_" + std::to_string(id);

  sem_unlink(getter_req_sem_name.c_str());
  sem_unlink(getter_res_sem_name.c_str());
  sem_unlink(adder_req_sem_name.c_str());

  getter_request_sem = sem_open(getter_req_sem_name.c_str(), O_CREAT, 0666, 0);
  getter_response_sem = sem_open(getter_res_sem_name.c_str(), O_CREAT, 0666, 0);
  adder_request_sem = sem_open(adder_req_sem_name.c_str(), O_CREAT, 0666, 0);


  if (getter_request_sem == SEM_FAILED || getter_response_sem == SEM_FAILED || adder_request_sem == SEM_FAILED) 
  {
      perror("Failed to open dispatcher semaphores");

      if (getter_request_sem != SEM_FAILED) sem_close(getter_request_sem);
      if (getter_response_sem != SEM_FAILED) sem_close(getter_response_sem);
      if (adder_request_sem != SEM_FAILED) sem_close(adder_request_sem);

      sem_unlink(getter_req_sem_name.c_str());
      sem_unlink(getter_res_sem_name.c_str());
      sem_unlink(adder_req_sem_name.c_str());

      exit(EXIT_FAILURE);

  }

  pthread_t adder, getter;

  assert(pthread_create(&adder, NULL, url_adder, NULL) == 0);
  assert(pthread_create(&getter, NULL, url_getter, NULL) == 0);

  pthread_t threads[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    sems[i] = sem_open(sem_names[i].data(), O_CREAT, 0666, 1);
    if (sems[i] == SEM_FAILED) assert(false);
    assert(pthread_create(threads + i, NULL, runner, (void*)(uint64_t)(i)) ==
           0);
  }
  std::cout << "STARTED RUNNER THREADS...\n";

  int num_joined = 0;
  const struct timespec timeout_limit = {.tv_sec = std::time(nullptr) + 60 * 15, .tv_nsec = 0};

  for (int i = 0; i < NUM_THREADS; i++) {
   // assert(pthread_join(threads[i], NULL) == 0);
   int res = pthread_timedjoin_np(threads[i], NULL, &timeout_limit);
   if (res) {
       std::cout << "Failed to join thread: " << i << '\n';
   }
   else {
	   ++num_joined;
   }
  }

  //  while (num_created) usleep(1000);

  // std::cout << "num created: " << num_created << std::endl;

  std::cout << "FINISHED RUNNER THREADS..." << std::endl;

  std::cout << "Waiting for add_to_index threads to complete..." << std::endl;

  size_t index_thread_count = add_to_index_threads.size();

  for (pthread_t handle : add_to_index_threads) 
  {
      int join_res = pthread_join(handle, NULL);

       if (join_res != 0) 
       {
           errno = join_res;
           perror("Failed to join add_to_index thread");
       }

  }

  std::cout << "Joined " << index_thread_count << " add_to_index threads." << std::endl;

  std::cout << "CRAWLER: Writing index chunks to files..." << std::endl;
  if (num_joined == NUM_THREADS) {
  // IndexFile chunkFile(id, chunk);
  for (int i = 0; i < NUM_CHUNKS; ++i) {
    pthread_lock_guard guard{chunk_locks[i]};
    IndexFile(id * NUM_CHUNKS + i, chunks[i]);
    // perror("Wrote with status: ");
  }

  std::cout << "Finished writing file" << std::endl;
  }
  std::cout << "Cleaning up semaphores..." << std::endl;

  for (int i = 0; i < NUM_THREADS; ++i) {
    if (sems[i] != SEM_FAILED) {
       if (sem_close(sems[i]) == -1) {
           perror("sem_close failed for sems[i]");
       }
       if (sem_unlink(sem_names[i].data()) == -1) {

      if (errno != ENOENT) 
      {
        perror("sem_unlink failed for sems[i]");
      }
    }
  }

  // Close and unlink other semaphores
  if (queue_sem != SEM_FAILED) {
    if (sem_close(queue_sem) == -1) perror("sem_close failed for queue_sem");
    if (sem_unlink("/crawler_sem") == -1) ;
  }
  if (getter_request_sem != SEM_FAILED) {
    if (sem_close(getter_request_sem) == -1) perror("sem_close failed for getter_request_sem");
    if (sem_unlink("/getter_request_sem") == -1) ;
  }
   if (getter_response_sem != SEM_FAILED) {
    if (sem_close(getter_response_sem) == -1) perror("sem_close failed for getter_response_sem");
    if (sem_unlink("/getter_response_sem") == -1) ; 
  }
   if (adder_request_sem != SEM_FAILED) {
    if (sem_close(adder_request_sem) == -1) perror("sem_close failed for adder_request_sem");
    if (sem_unlink("/adder_request_sem") == -1) ;
  }

  std::cout << "Semaphore cleanup finished." << std::endl;

  pthread_mutex_destroy(&queue_lock);
  pthread_mutex_destroy(&chunk_lock);
  pthread_mutex_destroy(&cout_lock);
  pthread_mutex_destroy(&getter_lock);
  pthread_mutex_destroy(&adder_lock);
  pthread_mutex_destroy(&add_to_index_threads_lock);
  
   for (int i = 0; i < NUM_CHUNKS; ++i) {
      pthread_mutex_destroy(chunk_locks + i);
  }


  // std::cout << "Time taken: " << duration.count() << " ms" << std::endl;
  return 0;
}
}

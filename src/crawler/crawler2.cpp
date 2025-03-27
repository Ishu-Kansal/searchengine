#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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
#include "../ranker/rank.h"
#include "sockets.h"
// #include "../inverted_index/Index.h"

constexpr uint32_t MAX_PROCESSED = 10000;
constexpr uint32_t MAX_VECTOR_SIZE = 50000;
constexpr uint32_t MAX_QUEUE_SIZE = 100000;
constexpr uint32_t TOP_K_ELEMENTS = 7500;
constexpr uint32_t NUM_RANDOM = 10000;

const static int NUM_THREADS = 32;  // start small

uint32_t STATIC_RANK = 0;  // temp global variable

std::queue<std::string> explore_queue{};
std::vector<std::pair<std::string, uint32_t>> links_vector;
Bloomfilter bf(100000, 0.0001);  // Temp size and false pos rate

pthread_mutex_t queue_lock{};
sem_t* queue_sem;
uint32_t num_processed{};
std::mt19937 mt{std::random_device{}()};

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
  // std::cout << "Enter fill_queue()" << std::endl;
  uint32_t links_vector_size = links_vector.size();

  if (explore_queue.empty() && links_vector_size > MAX_VECTOR_SIZE) {
    // std::cout << "Here1" << std::endl;
    // Establish range for uniform random num gen
    // Range is from 0 to the last element in the vector
    std::uniform_int_distribution<> gen{0, int(links_vector_size - 1)};
    // std::cout << "Here2" << std::endl;

    // Generates N random elements and moves them to the end
    for (size_t t = links_vector_size - 1; t > links_vector_size - NUM_RANDOM;
         t--) {
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
  sem_wait(queue_sem);
  pthread_lock_guard guard{queue_lock};

  size_t links_vector_size = links_vector.size();
  std::string url;

  if (!explore_queue.empty()) {
    // std::cout << "Pull url from explore queue" << std::endl;
    url = std::move(explore_queue.front());
    explore_queue.pop();
  } else if (links_vector_size > MAX_VECTOR_SIZE) {
    // std::cout << "Explore queue empty, fill queue with links from vector"
    // << std::endl;
    fill_queue();
    url = std::move(explore_queue.front());
    explore_queue.pop();
  } else {
    // std::cout << "Explore queue empty, use last link from vector"
    // << std::endl;
    url = std::move(links_vector.back().first);
    links_vector.pop_back();
  }

  return url;
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

void* runner(void*) {
  while (num_processed < MAX_PROCESSED) {
    // Get the next url to be processed
    std::string url = get_next_url();

    // Print the url that is being processed
    // std::cout << url << std::endl;

    // std::cout << "URL length: " << url.size() << std::endl;
    // std::cout << "Size of link vector: " << links_vector.size()
    // << std::endl;

    pthread_t thread;

    ThreadArgs args = {url, "", -1};

    // Start getHTML in a new thread
    pthread_create(&thread, nullptr, getHTML_wrapper, &args);
    pthread_join(thread, nullptr);

    // Continue if html code was not retrieved
    if (args.status != 0) {
      // std::cout << "Status " << args.status << std::endl;
      // std::cout << "Could not retrieve HTML\n" << std::endl;
      continue;
    }

    // Parse the html code
    std::string& html = args.html;
    HtmlParser parser(html.data(), html.size());

    // ------------------------------------------------------------------
    // TODO: The code to add the words from parser to the index goes here

    // ------------------------------------------------------------------

    // Process links found by the parser
    {
      auto static_rank = get_static_rank(cstring_view{url}, parser);
      pthread_lock_guard guard{queue_lock};
      num_processed++;
      if (num_processed % 1000 == 0) std::cout << num_processed << std::endl;

      if (links_vector.size() < MAX_QUEUE_SIZE) {
        for (auto& link : parser.links) {
          std::string next_url = std::move(link.URL);

          // Ignore links that begin with '#' or '?'
          if (next_url[0] == '#' || next_url[0] == '?') {
            continue;
          }

          // If link starts with '/', add the domain to the beginning of
          // it
          if (next_url[0] == '/') {
            next_url = url.substr(0, 8) + getHostFromUrl(url) + next_url;
          }

          if (!check_url(next_url)) {
            continue;
          }

          // If link has not been seen before, add it to the bf and links
          // vector
          if (!bf.contains(next_url)) {
            bf.insert(next_url);
            links_vector.emplace_back(std::move(next_url),
                                      static_rank);  // STATIC_RANK++});
            sem_post(queue_sem);
          }
        }
      }
    }

    // --------------------------------------------------
    // For debugging (not needed for crawler to function)
    /*std::string filename =
        "./files/file" + std::to_string(num_processed) + ".txt";
    std::ofstream output_file(filename);

    if (!output_file) {
      // std::cerr << "Error opening file!\n" << std::endl;
      // std::cerr << url << std::endl;
      continue;
    }

    output_file << url << "\n\n";
    output_file << "Number of links in queue: "
                << explore_queue.size() + links_vector.size() << "\n\n";
    output_file << parser.words.size() << " words\n";
    output_file << parser.links.size() << " links\n\n";
    output_file << html;
    output_file << "\n\n";
    for (auto& word : parser.words) output_file << word << ' ';

    output_file.close();*/
    // --------------------------------------------------

    // std::cout << '\n';
  }

  return NULL;
}

int main(int argc, char** argv) {
  std::vector<std::string> seed_urls = {
      "https://en.wikipedia.org/wiki/University_of_Michigan",
      "https://www.cnn.com",
      "https://www.reddit.com/",
      "https://cse.engin.umich.edu/",
      "https://stackoverflow.com/questions",
      "https://www.usa.gov/",
      "https://www.investopedia.com/",
      "https://www.nationalgeographic.com/",
      "https://www.nytimes.com/",
      "https://www.espn.com/",
      "https://weather.com/",
      "https://www.npr.org/",
      "https://www.apnews.com/",
      "https://www.tripadvisor.com/",
      "https://www.dictionary.com/",
      "https://www.foxnews.com/"
      "https://umich.edu/",
      "https://www.fandom.com/",
      "https://www.bing.com/"};

  for (const auto& url : seed_urls) {
    explore_queue.push(url);
    bf.insert(url);
  }
  sem_unlink("./crawler_sem");
  queue_sem = sem_open("./crawler_sem", O_CREAT, 0666, explore_queue.size());
  if (queue_sem == SEM_FAILED) exit(EXIT_FAILURE);

  auto start = std::chrono::high_resolution_clock::now();

  pthread_mutex_init(&queue_lock, NULL);
  pthread_t threads[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_create(threads + i, NULL, runner, NULL);
  }
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  pthread_mutex_destroy(&queue_lock);

  // Stop measuring time
  auto stop = std::chrono::high_resolution_clock::now();

  // Calculate duration in milliseconds
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

  // std::cout << "Time taken: " << duration.count() << " ms" << std::endl;
  sem_close(queue_sem);
  return 0;
}
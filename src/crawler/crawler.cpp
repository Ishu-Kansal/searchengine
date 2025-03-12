#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <utility>

#include "BloomFilterStarterFiles/BloomFilter.h"
#include "HtmlParser/HtmlParser.h"
#include "utils/pthread_lock_guard.h"

const uint32_t MAX_PROCESSED = 5;
const uint32_t TOP_K_ELEMENTS = 5000;
uint32_t STATIC_RANK = 0; //temp global variable

std::queue<std::string> explore_queue{};
std::vector<std::pair<std::string, uint32_t>> links_vector;
Bloomfilter bf(100000, .0001);  // Temp size and false pos rate
sem_t *queue_sem{};

pthread_mutex_t queue_lock{};
pthread_mutex_t output_lock{};
uint32_t num_processed{};

int get_and_parse_url(const char *url, int fd) {
  static const char *const proc = "searchengine/LinuxGetUrl/LinuxGetUrl";

  pid_t pid = fork();
  if (pid == 0) {
    dup2(fd, STDOUT_FILENO);  // redirect STDOUT to output file
    int t = execl(proc, proc, url, NULL);
    if (t == -1) {
      std::clog << "Failed to execute for url: " << url << '\n';
      return -1;
    }
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (status != 0) {
      std::clog << "Failed reading for url: " << url << '\n';
    }
    return status;
  }
  return 0;
}

int get_file_size(int fd) {
  struct stat buf;
  fstat(fd, &buf);
  return buf.st_size;
}

int partition(int left, int right, int pivot_index) 
{
  int pivot_rank = links_vector[pivot_index].second;
  // Move the pivot to the end
  std::swap(links_vector[pivot_index], links_vector[right]);

  // Move all less ranked elements to the left
  int store_index = left;
  for (int i = left; i <= right; i++) {
      if (links_vector[i].second < pivot_rank) {
          std::swap(links_vector[store_index], links_vector[i]);
          store_index += 1;
      }
  }

  // Move the pivot to its final place
  std::swap(links_vector[right], links_vector[store_index]);

  return store_index;
}

void quickselect(int left, int right, int k) 
{

  int pivot_index = left + rand() % (right - left + 1);

  // Find the pivot position in a sorted list
  pivot_index = partition(left, right, pivot_index);

  //If the pivot is in its final sorted position
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

void fill_queue()
{
  uint32_t links_vector_size = links_vector.size();
  if (explore_queue.empty() && links_vector_size >= 10000)
  {
    quickselect(0, links_vector_size - 1, TOP_K_ELEMENTS);
    for (size_t i = links_vector_size - 1; i > links_vector_size - TOP_K_ELEMENTS; --i)
    {
      explore_queue.push(std::move(links_vector[i].first));
      links_vector.pop_back();
    }
  }
}
#include <string>
#include <vector>
#include <queue>
#include <pthread.h>

std::string get_next_url()
{
    pthread_mutex_lock(&queue_lock);

    size_t links_vector_size = links_vector.size();
    std::string url;

    if (!explore_queue.empty())
    {
        url = std::move(explore_queue.front());
        explore_queue.pop();
    }
    else if (links_vector_size > 10000)
    {
        fill_queue();
        url = std::move(explore_queue.front());
        explore_queue.pop();
    }
    else
    {
        url = std::move(links_vector[links_vector_size - 1].first);
        links_vector.pop_back();
    }

    pthread_mutex_unlock(&queue_lock);

    return url;
}

void *runner(void *) {
  while (num_processed < MAX_PROCESSED) { 

    // sem_wait(queue_sem);

    std::string url = get_next_url();

    const std::string fileName = "../files/" + url + ".txt";

    int outputFd = open(fileName.data(), O_CREAT | O_TRUNC | O_WRONLY | O_EXCL);
    if (outputFd == -1) {
      std::clog << "URL already processed for url: " << url << '\n';
      continue;
    }

    int status = get_and_parse_url(url.data(), outputFd);
    if (status != 0) {
      close(outputFd);
      continue;
    }
    const int len = get_file_size(outputFd);

    const char *fileData =
        (char *)mmap(nullptr, len, O_RDONLY, PROT_READ, outputFd, 0);
    if (fileData == MAP_FAILED) {
      std::clog << "Failed to process url: " << url << '\n';
      close(outputFd);
      continue;
    }
    try {
      HtmlParser parser(fileData, len);
      ++num_processed;
      {
        pthread_lock_guard guard{queue_lock};
        for (auto &link : parser.links) {
          std::string url = std::move(link.URL);
          if (!bf.contains(url)) {
            bf.insert(url);
            links_vector.push_back({std::move(url), STATIC_RANK++});
        //    sem_post(queue_sem);
          }
        }
      }
      // add to index
      
    } catch (...) {
    }
    munmap((void *)fileData, len);
    close(outputFd);
  }
  return NULL;
}

int main(int, char **) {
  std::ofstream logging_file{"log.txt"};
  std::clog.rdbuf(logging_file.rdbuf());

  pthread_mutex_init(&queue_lock, NULL);
  queue_sem = sem_open("/crawler_semaphore", O_CREAT);
  if (queue_sem == SEM_FAILED) {
    std::clog << "Failed to open semaphore: " << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  const static int num_threads = 10;  // start small
  pthread_t threads[num_threads];
  for (int i = 0; i < num_threads; ++i) {
    pthread_create(threads + i, NULL, runner, NULL);
  }
  for (int i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  sem_close(queue_sem);
  pthread_mutex_destroy(&queue_lock);
}

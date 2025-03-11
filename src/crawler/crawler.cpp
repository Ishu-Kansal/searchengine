#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <queue>
#include <string>

#include "BloomFilterStarterFiles/BloomFilter.h"
#include "HtmlParser/HtmlParser.h"
#include "utils/pthread_lock_guard.h"

const uint32_t MAX_PROCESSED = 5;

std::queue<std::string> explore_queue{};
Bloomfilter bf(100000, .0001);  // Temp size and false pos rate
sem_t *queue_sem{};
pthread_rwlock_t bf_lock{};
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
}

int get_file_size(int fd) {
  struct stat buf;
  fstat(fd, &buf);
  return buf.st_size;
}

void *runner(void *) {
  while (num_processed < MAX_PROCESSED) {
    sem_wait(queue_sem);

    pthread_mutex_lock(&queue_lock);
    const std::string url = std::move(explore_queue.front());
    explore_queue.pop();
    pthread_mutex_unlock(&queue_lock);

    const std::string fileName = "../files/" + std::string(url) + ".txt";

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
        for (const auto &link : parser.links) {
          const std::string url = std::move(link.URL);
          if (!bf.contains(url)) {
            bf.insert(url);
            explore_queue.push(std::move(url));
            sem_post(queue_sem);
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

  const static int num_threads = 10;  // start small
  pthread_t threads[num_threads];
  for (int i = 0; i < num_threads; ++i) {
    pthread_create(threads + i, NULL, runner, NULL);
  }

  sem_close(queue_sem);
  pthread_mutex_destroy(&queue_lock);
}

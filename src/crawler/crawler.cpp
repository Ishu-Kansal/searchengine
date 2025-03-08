#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <queue>
#include <string>

const uint32_t MAX_PROCESSED = 5;

std::queue<std::string> explore_queue{};
// bloom filter
sem_t *queue_sem{};
pthread_mutex_t queue_lock{};
uint32_t num_processed{};

void get_and_parse_url(const char *url, int fd) {
  static const char *const proc = "../../LinuxGetUrl/LinuxGetUrl";

  pid_t pid = fork();
  if (pid == 0) {
    dup2(fd, STDOUT_FILENO);  // redirect STDOUT to output file
    execl(proc, proc, url, NULL);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (status != 0) {
      std::clog << "Failed reading for url: " << url << '\n';
      return;
    }
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

    const std::string fileName = "/files/" + std::string(url) + ".txt";

    int outputFd = open(fileName.data(), O_CREAT | O_TRUNC | O_WRONLY | O_EXCL);
    if (outputFd == -1) {
      std::clog << "URL already processed for url: " << url << '\n';
      continue;
    }

    get_and_parse_url(url.data(), outputFd);

    const char *fileData = (char *)mmap(nullptr, get_file_size(outputFd),
                                        O_RDONLY, PROT_READ, outputFd, 0);
    // use fileData as input for html parser
  }
  return NULL;
}

int main(int, char **) {
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

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
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
  const static const char *proc = "../../LinuxGetUrl/LinuxGetUrl";

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

void runner() {
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
      return;
    }

    get_and_parse_url(url.data(), outputFd);
  }
}

int main(int, char **) {
  pthread_mutex_init(&queue_lock, NULL);
  queue_sem = sem_open("/crawler_semaphore", O_CREAT);

  sem_close(queue_sem);
  pthread_mutex_destroy(&queue_lock);
}

#include <pthread.h>
#include <unistd.h>

#include <iostream>
#include <string>

void *spawner(void *arg) {
  const uint32_t id = (uint64_t)(arg);
  const std::string id_str = std::to_string(id);
  while (true) {
    if (pid_t pid = fork()) {
      int status;
      waitpid(pid, &status, 0);
      if (status == 0) break;
    } else {
      execl("./crawler", "crawler", id_str.c_str(), NULL);
    }
  }
  return NULL;
}

void *create_dispatcher(void *) {
  if (fork() == 0) {
    execl("./dispatcher", "dispatcher");
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: ./script [num_crawlers]";
    return 1;
  }

  pthread_t dispatcher;
  pthread_create(&dispatcher, NULL, create_dispatcher, NULL);
  pthread_detach(dispatcher);

  const int NUM_CRAWLERS = atoi(argv[1]);
  pthread_t threads[NUM_CRAWLERS];
  for (uint64_t i = 0; i < NUM_CRAWLERS; ++i) {
    pthread_create(threads + i, NULL, spawner, (void *)(i));
  }
  for (uint64_t i = 0; i < NUM_CRAWLERS; ++i) {
    pthread_join(threads[i], NULL);
  }
}
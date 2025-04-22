#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

#include "constants.h"

sem_t *sem{};

void *spawner(void *arg) {
  const uint32_t id = (uint64_t)(arg);
  const std::string id_str = std::to_string(id);
  while (true) {
    if (pid_t pid = fork()) {
      int status;
      waitpid(pid, &status, 0);
      if (status == 0) break;
    } else {
      execl("./crawler", "crawler", id_str.c_str(), "127.0.0.1", NULL);
    }
  }
  sem_post(sem);
  return NULL;
}

void *create_dispatcher(void *) {
  while (true) {
    if (pid_t pid = fork()) {
      int status;
      waitpid(pid, &status, 0);
      if (status == 0) break;
    } else {
      execl("./dispatcher", "dispatcher", NULL);
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: ./script [num_batches] [crawlers_per_batch]";
    return 1;
  }

  pthread_t dispatcher;
  pthread_create(&dispatcher, NULL, create_dispatcher, NULL);

  sleep(2);

  const int NUM_BATCHES = atoi(argv[1]);
  const int CRAWLERS_PER_BATCH = atoi(argv[2]);
  const int TOTAL = NUM_BATCHES * CRAWLERS_PER_BATCH;

  // assert(TOTAL * MAX_PROCESSED == 10'000'000);
  std::cout << "Total processed: " << TOTAL * MAX_PROCESSED << '\n';

  pthread_t threads[TOTAL];

  sem_unlink("/script_sem");
  sem = sem_open("/script_sem", O_CREAT, 0666, CRAWLERS_PER_BATCH);
  assert(sem != SEM_FAILED);

  for (int i = 0; i < TOTAL; ++i) {
    sem_wait(sem);
    pthread_create(threads + i, NULL, spawner, (void *)(i));
  }

  for (uint64_t i = 0; i < TOTAL; ++i) {
    pthread_join(threads[i], NULL);
  }

  pthread_join(dispatcher, NULL);
}

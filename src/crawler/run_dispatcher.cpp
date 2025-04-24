#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

void *create_dispatcher(void *) {
  while (true) {
    if (pid_t pid = fork()) {
      int status;
      waitpid(pid, &status, 0);
      if (status == 0) break;
    } else {
      execl("./dispatcher", "dispatcher", NULL);
      // kill all processes starting with crawler*
    }
  }
  return NULL;
}

int main() {
  pthread_t t;
  pthread_create(&t, NULL, create_dispatcher, NULL);
  pthread_detach(t);
}
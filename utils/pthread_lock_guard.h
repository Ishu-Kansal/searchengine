#pragma once

#include <pthread.h>

class pthread_lock_guard {
  pthread_lock_guard(pthread_mutex_t &lock) : lock{lock} {
    pthread_mutex_lock(&lock);
  }

  ~pthread_lock_guard() { pthread_mutex_unlock(&lock); }

 private:
  pthread_mutex_t &lock;
};

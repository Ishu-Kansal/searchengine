#pragma once

#include <pthread.h>

struct adopt_lock_t {};

class pthread_lock_guard {
 public:
  explicit pthread_lock_guard(pthread_mutex_t &lock) : lock{lock} {
    pthread_mutex_lock(&lock);
  }

  pthread_lock_guard(pthread_mutex_t &lock, adopt_lock_t) : lock{lock} {}
  pthread_lock_guard(const pthread_mutex_t &) = delete;
  pthread_lock_guard &operator=(const pthread_mutex_t &) = delete;

  ~pthread_lock_guard() { pthread_mutex_unlock(&lock); }

 private:
  pthread_mutex_t &lock;
};

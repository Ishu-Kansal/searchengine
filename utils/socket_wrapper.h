#pragma once
#include <unistd.h>

#include <cassert>

struct SocketWrapper {
   public:
    explicit SocketWrapper(int fd) : fd_{fd} { assert(fd != -1); }
    ~SocketWrapper() { close(fd_); }

   private:
    int fd_;
};
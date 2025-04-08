#pragma once

#include <string>

#include "../../crawler/sockets.h"

struct ThreadArgs {
  std::string url;
  std::string html;
  int status;
};

void* getHTML_wrapper(void* arg) {
  ThreadArgs* args = (ThreadArgs*)arg;

  args->status = getHTML(args->url, args->html);

  return nullptr;
}

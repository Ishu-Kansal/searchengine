// #include <google/cloud/functions/framework.h>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "../../HtmlParser/HtmlParser.h"
#include "../../utils/cstring_view.h"
#include "../crawler/sockets.h"
#include "../inverted_index/Index.h"
#include "../inverted_index/IndexFile.h"
#include "../ranker/rank.h"
#include "info.grpc.pb.h"
#include "utils.h"

using cloudcrawler::AddRequest;
using cloudcrawler::Dispatcher;
using cloudcrawler::Empty;
using cloudcrawler::GetResponse;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

uint64_t CRAWL_LIMIT = 100;
const char *const server = getenv("DISPATCHER_ADDRESS");
//    "grpc.client.dispatcher.address=dns://dispatcher";

struct AddArgs {
  ClientContext context;
  AddRequest request;
  Empty response;
};

AddArgs *CreateRequest(std::string &s, uint32_t rank) {
  AddArgs *args = new AddArgs;
  std::chrono::system_clock::time_point deadline =
      std::chrono::system_clock::now() + std::chrono::seconds(5);
  args->context.set_deadline(deadline);
  args->request.set_url(std::move(s));
  args->request.set_rank(rank);
  return args;
}

class Crawler {
public:
  Crawler(std::shared_ptr<Channel> channel, uint32_t id)
      : stub_{Dispatcher::NewStub(channel)} {
    std::string get_sem_name = "/get_sem" + std::to_string(id);
    std::string post_sem_name = "/post_sem" + std::to_string(id);
    sem_unlink(get_sem_name.data());
    get_sem = sem_open(get_sem_name.data(), O_CREAT, 0666, 0);
    sem_unlink(post_sem_name.data());
    post_sem = sem_open(post_sem_name.data(), O_CREAT, 0666, 0);
    assert(get_sem && post_sem);
  }

  ~Crawler() {
    sem_close(get_sem);
    sem_close(post_sem);
  }

  void run() {
    std::vector<AddArgs *> add_args{};
    while (num_processed < CRAWL_LIMIT) {
      std::string url = get_url();
      if (url.empty()) {
        sleep(3);
        continue;
      }
      std::clog << "Processing: " << url
                << " with number of links processed: " << num_processed
                << std::endl;

      ThreadArgs args = {.url = url, .html = "", .status = -1};

      // Start getHTML in a new thread
      pthread_t thread;
      int res1 = pthread_create(&thread, nullptr, getHTML_wrapper, &args);
      assert(res1 == 0);
      res1 = pthread_join(thread, nullptr);
      assert(res1 == 0);

      if (args.status != 0) {
        std::clog << "Got status: " << args.status << std::endl;
      };

      HtmlParser parser(args.html.data(), args.html.size());
      uint32_t rank = get_static_rank(url, parser);

      ++num_processed;
      int posted = 0;
      for (auto &link : parser.links) {
        add_args.push_back(CreateRequest(link.URL, rank));
        posted += add_url(url, add_args.back());
      }

      chunk.add_url(url, rank);

      chunk.add_enddoc();

      // for (int i = 0; i < posted; ++i) sem_wait(post_sem);
      for (auto ptr : add_args)
        delete ptr;
      add_args.clear();
    }
  }

private:
  std::string get_url() const {
    Empty req;
    GetResponse response;
    ClientContext context;
    std::chrono::system_clock::time_point deadline =
        std::chrono::system_clock::now() + std::chrono::seconds(5);
    context.set_deadline(deadline);
    GetResponse result{};

    stub_->async()->GetUrl(
        &context, &req, &response,
        [&result, &req, &context, &response, this](Status status) mutable {
          if (!status.ok()) {
            response.set_url("");
          }
          sem_post(get_sem);
        });
    sem_wait(get_sem);
    return response.url();
  }

  int add_url(const std::string &url, AddArgs *args) const {
    std::string &next_url = *args->request.mutable_url();
    if (next_url[0] == '#' || next_url[0] == '?')
      return 0;

    // If link starts with '/', add the domain to the beginning of
    // it
    if (next_url[0] == '/')
      next_url = url.substr(0, 8) + getHostFromUrl(url) + next_url;

    if (!check_url(next_url))
      return 0;
    stub_->async()->AddUrl(&args->context, &args->request, &args->response,
                           [args, this](Status status) { sem_post(post_sem); });
    sem_wait(post_sem);
    return 1;
  }

  bool check_url(cstring_view url) const {
    // If link does not begin with 'http', ignore it
    if (url.size() < 4 || !url.starts_with(cstring_view{"http", 4UZ})) {
      return false;
    }

    if (url.find(cstring_view{"porn"}) != cstring_view::npos)
      return false;

    if (url.size() > 250) {
      return false;
    }

    return true;
  }

  std::string getHostFromUrl(const std::string &url) const {
    // std::regex urlRe("^.*://([^/?:]+)/?.*$");
    // return std::regex_replace(url, urlRe, "$1");

    std::string::size_type pos = url.find("://");
    if (pos == std::string::npos)
      return "";
    pos += 3;
    std::string::size_type endPos = url.find('/', pos);
    return url.substr(pos, endPos - pos);
  }

private:
  std::unique_ptr<Dispatcher::Stub> stub_{};
  IndexChunk chunk{};
  uint64_t num_processed{};

  sem_t *get_sem{};
  sem_t *post_sem{};
};

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);
  if (argc != 2 && argc != 3) {
    std::cout << "USAGE: ./crawler [ID] [LIMIT]" << std::endl;
    return 1;
  }
  int id = atoi(argv[1]);
  if (argc == 3)
    CRAWL_LIMIT = atoi(argv[2]);
  Crawler crawl(grpc::CreateChannel(server, grpc::InsecureChannelCredentials()),
                id);
  crawl.run();
  std::cout << "WE DID IT!!!" << std::endl;
}

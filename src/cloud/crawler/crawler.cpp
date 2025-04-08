#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <semaphore.h>
#include <time.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "../../../HtmlParser/HtmlParser.h"
#include "../../../utils/cstring_view.h"
#include "../../crawler/sockets.h"
#include "../../inverted_index/Index.h"
#include "../../inverted_index/IndexFile.h"
#include "../../ranker/rank.h"
#include "../info.grpc.pb.h"
#include "utils.h"

using cloudcrawler::AddRequest;
using cloudcrawler::Dispatcher;
using cloudcrawler::Empty;
using cloudcrawler::GetResponse;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

constexpr uint64_t CRAWL_LIMIT = 100;
constexpr const char* const server = "localhost:50051";

AddRequest CreateRequest(std::string& s, uint32_t rank) {
  AddRequest request;
  request.set_url(std::move(s));
  request.set_rank(rank);
  return request;
}

class Crawler {
 public:
  Crawler(std::shared_ptr<Channel> channel, uint32_t id)
      : stub_{Dispatcher::NewStub(channel)} {
    std::chrono::system_clock::time_point deadline =
        std::chrono::system_clock::now() + std::chrono::milliseconds(100);
    context.set_deadline(deadline);
    std::string get_sem_name = "./get_sem" + std::to_string(id);
    std::string post_sem_name = "./post_sem" + std::to_string(id);
    sem_unlink(get_sem_name.data());
    get_sem = sem_open(get_sem_name.data(), O_CREAT, 0666, 0);
    sem_unlink(post_sem_name.data());
    post_sem = sem_open(post_sem_name.data(), O_CREAT, 0666, 0);
  }

  void run() {
    while (num_processed < CRAWL_LIMIT) {
      std::string url = get_url();
      ThreadArgs args = {url, "", -1};

      // Start getHTML in a new thread
      pthread_t thread;
      pthread_create(&thread, nullptr, getHTML_wrapper, &args);
      pthread_join(thread, nullptr);

      if (args.status != 0) continue;

      HtmlParser parser(args.html.data(), args.html.size());
      uint32_t rank = get_static_rank(url, parser);

      ++num_processed;
      for (auto& link : parser.links) {
        add_url(url, link.URL, rank);
      }
    }
  }

 private:
  std::string get_url() {
    Empty req{};
    GetResponse response{};
    std::string result{};

    stub_->async()->GetUrl(&context, &req, &response,
                           [&result, &response, this](Status status) mutable {
                             if (!status.ok()) {
                               std::clog << "Failed to get url" << std::endl;
                               assert(false);
                             }
                             result = response.url();
                             std::clog << "Obtained url: " << result << '\n';
                             sem_post(get_sem);
                           });
    sem_wait(get_sem);
    return result;
  }

  void add_url(const std::string& url, std::string& next_url, uint32_t rank) {
    if (next_url[0] == '#' || next_url[0] == '?') return;

    // If link starts with '/', add the domain to the beginning of
    // it
    if (next_url[0] == '/')
      next_url = url.substr(0, 8) + getHostFromUrl(url) + next_url;

    if (!check_url(next_url)) return;

    Empty response{};
    AddRequest request = CreateRequest(next_url, rank);
    stub_->async()->AddUrl(
        &context, &request, &response, [&, this](Status status) {
          if (!status.ok()) {
            assert(false);
          }
          std::clog << "Added url: " << request.url() << '\n';
          sem_post(post_sem);
        });
    sem_wait(post_sem);
  }

  bool check_url(cstring_view url) {
    // If link does not begin with 'http', ignore it
    if (url.size() < 4 || !url.starts_with(cstring_view{"http", 4UZ})) {
      return false;
    }

    if (url.find(cstring_view{"porn"}) != cstring_view::npos) return false;

    if (url.size() > 250) {
      return false;
    }

    return true;
  }

  std::string getHostFromUrl(const std::string& url) {
    // std::regex urlRe("^.*://([^/?:]+)/?.*$");
    // return std::regex_replace(url, urlRe, "$1");

    std::string::size_type pos = url.find("://");
    if (pos == std::string::npos) return "";
    pos += 3;
    std::string::size_type endPos = url.find('/', pos);
    return url.substr(pos, endPos - pos);
  }

 private:
  ClientContext context{};
  std::unique_ptr<Dispatcher::Stub> stub_{};
  IndexChunk chunk{};
  uint64_t num_processed = 0;

  sem_t* get_sem{};
  sem_t* post_sem{};
};

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "USAGE: ./crawler [ID]" << std::endl;
    return 1;
  }
  int id = atoi(argv[1]);
  Crawler crawl(grpc::CreateChannel(server, grpc::InsecureChannelCredentials()),
                id);
  crawl.run();
  std::cout << "WE DID IT!!!" << std::endl;
}

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <cstdint>
#include <string>

#include "../../../utils/cstring_view.h"
#include "../../inverted_index/Index.h"
#include "../../inverted_index/IndexFile.h"
#include "../info.grpc.pb.h"

using cloudcrawler::AddRequest;
using cloudcrawler::Empty;
using cloudcrawler::GetResponse;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

AddRequest CreateRequest(std::string &s, uint32_t rank) {
  AddRequest request;
  request.set_url(std::move(s));
  request.set_rank(rank);
  return request;
}

class Crawler {
 private:
  uint64_t num_crawled = 0;
};
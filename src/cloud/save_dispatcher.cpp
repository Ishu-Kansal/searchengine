#include <fcntl.h>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <semaphore.h>
#include <unistd.h>

#include <iostream>
#include <memory>

#include "info.grpc.pb.h"

using cloudcrawler::Dispatcher;
using cloudcrawler::Empty;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

constexpr const char *const server = "localhost:50051";

int main() {
  sem_t *kill_sem;
  std::string sem_name = "./kill_sem";
  sem_unlink(sem_name.data());
  kill_sem = sem_open(sem_name.data(), O_CREAT, 0666, 0);
  std::shared_ptr<Channel> ptr =
      grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
  std::unique_ptr<Dispatcher::Stub> stub{Dispatcher::NewStub(ptr)};
  Empty req, resp;
  ClientContext context;
  stub->async()->SaveService(&context, &req, &resp, [&](Status status) {
    if (!status.ok()) {
      std::clog << "FAILED TO SAVE FILE: RERUN AGAIN";
    }
    sem_post(kill_sem);
  });
  sem_wait(kill_sem);
}
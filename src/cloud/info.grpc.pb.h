// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: info.proto
#ifndef GRPC_info_2eproto__INCLUDED
#define GRPC_info_2eproto__INCLUDED

#include "info.pb.h"

#include <functional>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/proto_utils.h>
#include <grpcpp/impl/rpc_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/stub_options.h>
#include <grpcpp/support/sync_stream.h>
#include <grpcpp/ports_def.inc>

namespace cloudcrawler {

class Dispatcher final {
 public:
  static constexpr char const* service_full_name() {
    return "cloudcrawler.Dispatcher";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status GetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::cloudcrawler::GetResponse* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::GetResponse>> AsyncGetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::GetResponse>>(AsyncGetUrlRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::GetResponse>> PrepareAsyncGetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::GetResponse>>(PrepareAsyncGetUrlRaw(context, request, cq));
    }
    virtual ::grpc::Status AddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::cloudcrawler::Empty* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::Empty>> AsyncAddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::Empty>>(AsyncAddUrlsRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::Empty>> PrepareAsyncAddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::Empty>>(PrepareAsyncAddUrlsRaw(context, request, cq));
    }
    class async_interface {
     public:
      virtual ~async_interface() {}
      virtual void GetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty* request, ::cloudcrawler::GetResponse* response, std::function<void(::grpc::Status)>) = 0;
      virtual void GetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty* request, ::cloudcrawler::GetResponse* response, ::grpc::ClientUnaryReactor* reactor) = 0;
      virtual void AddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest* request, ::cloudcrawler::Empty* response, std::function<void(::grpc::Status)>) = 0;
      virtual void AddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest* request, ::cloudcrawler::Empty* response, ::grpc::ClientUnaryReactor* reactor) = 0;
    };
    typedef class async_interface experimental_async_interface;
    virtual class async_interface* async() { return nullptr; }
    class async_interface* experimental_async() { return async(); }
   private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::GetResponse>* AsyncGetUrlRaw(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::GetResponse>* PrepareAsyncGetUrlRaw(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::Empty>* AsyncAddUrlsRaw(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::cloudcrawler::Empty>* PrepareAsyncAddUrlsRaw(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());
    ::grpc::Status GetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::cloudcrawler::GetResponse* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::cloudcrawler::GetResponse>> AsyncGetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::cloudcrawler::GetResponse>>(AsyncGetUrlRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::cloudcrawler::GetResponse>> PrepareAsyncGetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::cloudcrawler::GetResponse>>(PrepareAsyncGetUrlRaw(context, request, cq));
    }
    ::grpc::Status AddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::cloudcrawler::Empty* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::cloudcrawler::Empty>> AsyncAddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::cloudcrawler::Empty>>(AsyncAddUrlsRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::cloudcrawler::Empty>> PrepareAsyncAddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::cloudcrawler::Empty>>(PrepareAsyncAddUrlsRaw(context, request, cq));
    }
    class async final :
      public StubInterface::async_interface {
     public:
      void GetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty* request, ::cloudcrawler::GetResponse* response, std::function<void(::grpc::Status)>) override;
      void GetUrl(::grpc::ClientContext* context, const ::cloudcrawler::Empty* request, ::cloudcrawler::GetResponse* response, ::grpc::ClientUnaryReactor* reactor) override;
      void AddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest* request, ::cloudcrawler::Empty* response, std::function<void(::grpc::Status)>) override;
      void AddUrls(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest* request, ::cloudcrawler::Empty* response, ::grpc::ClientUnaryReactor* reactor) override;
     private:
      friend class Stub;
      explicit async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class async* async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::cloudcrawler::GetResponse>* AsyncGetUrlRaw(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::cloudcrawler::GetResponse>* PrepareAsyncGetUrlRaw(::grpc::ClientContext* context, const ::cloudcrawler::Empty& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::cloudcrawler::Empty>* AsyncAddUrlsRaw(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::cloudcrawler::Empty>* PrepareAsyncAddUrlsRaw(::grpc::ClientContext* context, const ::cloudcrawler::AddRequest& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_GetUrl_;
    const ::grpc::internal::RpcMethod rpcmethod_AddUrls_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status GetUrl(::grpc::ServerContext* context, const ::cloudcrawler::Empty* request, ::cloudcrawler::GetResponse* response);
    virtual ::grpc::Status AddUrls(::grpc::ServerContext* context, const ::cloudcrawler::AddRequest* request, ::cloudcrawler::Empty* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_GetUrl : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_GetUrl() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_GetUrl() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetUrl(::grpc::ServerContext* /*context*/, const ::cloudcrawler::Empty* /*request*/, ::cloudcrawler::GetResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestGetUrl(::grpc::ServerContext* context, ::cloudcrawler::Empty* request, ::grpc::ServerAsyncResponseWriter< ::cloudcrawler::GetResponse>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_AddUrls : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_AddUrls() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_AddUrls() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status AddUrls(::grpc::ServerContext* /*context*/, const ::cloudcrawler::AddRequest* /*request*/, ::cloudcrawler::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestAddUrls(::grpc::ServerContext* context, ::cloudcrawler::AddRequest* request, ::grpc::ServerAsyncResponseWriter< ::cloudcrawler::Empty>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_GetUrl<WithAsyncMethod_AddUrls<Service > > AsyncService;
  template <class BaseClass>
  class WithCallbackMethod_GetUrl : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_GetUrl() {
      ::grpc::Service::MarkMethodCallback(0,
          new ::grpc::internal::CallbackUnaryHandler< ::cloudcrawler::Empty, ::cloudcrawler::GetResponse>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::cloudcrawler::Empty* request, ::cloudcrawler::GetResponse* response) { return this->GetUrl(context, request, response); }));}
    void SetMessageAllocatorFor_GetUrl(
        ::grpc::MessageAllocator< ::cloudcrawler::Empty, ::cloudcrawler::GetResponse>* allocator) {
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::GetHandler(0);
      static_cast<::grpc::internal::CallbackUnaryHandler< ::cloudcrawler::Empty, ::cloudcrawler::GetResponse>*>(handler)
              ->SetMessageAllocator(allocator);
    }
    ~WithCallbackMethod_GetUrl() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetUrl(::grpc::ServerContext* /*context*/, const ::cloudcrawler::Empty* /*request*/, ::cloudcrawler::GetResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* GetUrl(
      ::grpc::CallbackServerContext* /*context*/, const ::cloudcrawler::Empty* /*request*/, ::cloudcrawler::GetResponse* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithCallbackMethod_AddUrls : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_AddUrls() {
      ::grpc::Service::MarkMethodCallback(1,
          new ::grpc::internal::CallbackUnaryHandler< ::cloudcrawler::AddRequest, ::cloudcrawler::Empty>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::cloudcrawler::AddRequest* request, ::cloudcrawler::Empty* response) { return this->AddUrls(context, request, response); }));}
    void SetMessageAllocatorFor_AddUrls(
        ::grpc::MessageAllocator< ::cloudcrawler::AddRequest, ::cloudcrawler::Empty>* allocator) {
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::GetHandler(1);
      static_cast<::grpc::internal::CallbackUnaryHandler< ::cloudcrawler::AddRequest, ::cloudcrawler::Empty>*>(handler)
              ->SetMessageAllocator(allocator);
    }
    ~WithCallbackMethod_AddUrls() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status AddUrls(::grpc::ServerContext* /*context*/, const ::cloudcrawler::AddRequest* /*request*/, ::cloudcrawler::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* AddUrls(
      ::grpc::CallbackServerContext* /*context*/, const ::cloudcrawler::AddRequest* /*request*/, ::cloudcrawler::Empty* /*response*/)  { return nullptr; }
  };
  typedef WithCallbackMethod_GetUrl<WithCallbackMethod_AddUrls<Service > > CallbackService;
  typedef CallbackService ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_GetUrl : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_GetUrl() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_GetUrl() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetUrl(::grpc::ServerContext* /*context*/, const ::cloudcrawler::Empty* /*request*/, ::cloudcrawler::GetResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_AddUrls : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_AddUrls() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_AddUrls() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status AddUrls(::grpc::ServerContext* /*context*/, const ::cloudcrawler::AddRequest* /*request*/, ::cloudcrawler::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_GetUrl : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_GetUrl() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_GetUrl() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetUrl(::grpc::ServerContext* /*context*/, const ::cloudcrawler::Empty* /*request*/, ::cloudcrawler::GetResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestGetUrl(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_AddUrls : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_AddUrls() {
      ::grpc::Service::MarkMethodRaw(1);
    }
    ~WithRawMethod_AddUrls() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status AddUrls(::grpc::ServerContext* /*context*/, const ::cloudcrawler::AddRequest* /*request*/, ::cloudcrawler::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestAddUrls(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_GetUrl : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_GetUrl() {
      ::grpc::Service::MarkMethodRawCallback(0,
          new ::grpc::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response) { return this->GetUrl(context, request, response); }));
    }
    ~WithRawCallbackMethod_GetUrl() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetUrl(::grpc::ServerContext* /*context*/, const ::cloudcrawler::Empty* /*request*/, ::cloudcrawler::GetResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* GetUrl(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_AddUrls : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_AddUrls() {
      ::grpc::Service::MarkMethodRawCallback(1,
          new ::grpc::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response) { return this->AddUrls(context, request, response); }));
    }
    ~WithRawCallbackMethod_AddUrls() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status AddUrls(::grpc::ServerContext* /*context*/, const ::cloudcrawler::AddRequest* /*request*/, ::cloudcrawler::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* AddUrls(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_GetUrl : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_GetUrl() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler<
          ::cloudcrawler::Empty, ::cloudcrawler::GetResponse>(
            [this](::grpc::ServerContext* context,
                   ::grpc::ServerUnaryStreamer<
                     ::cloudcrawler::Empty, ::cloudcrawler::GetResponse>* streamer) {
                       return this->StreamedGetUrl(context,
                         streamer);
                  }));
    }
    ~WithStreamedUnaryMethod_GetUrl() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status GetUrl(::grpc::ServerContext* /*context*/, const ::cloudcrawler::Empty* /*request*/, ::cloudcrawler::GetResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedGetUrl(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::cloudcrawler::Empty,::cloudcrawler::GetResponse>* server_unary_streamer) = 0;
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_AddUrls : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_AddUrls() {
      ::grpc::Service::MarkMethodStreamed(1,
        new ::grpc::internal::StreamedUnaryHandler<
          ::cloudcrawler::AddRequest, ::cloudcrawler::Empty>(
            [this](::grpc::ServerContext* context,
                   ::grpc::ServerUnaryStreamer<
                     ::cloudcrawler::AddRequest, ::cloudcrawler::Empty>* streamer) {
                       return this->StreamedAddUrls(context,
                         streamer);
                  }));
    }
    ~WithStreamedUnaryMethod_AddUrls() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status AddUrls(::grpc::ServerContext* /*context*/, const ::cloudcrawler::AddRequest* /*request*/, ::cloudcrawler::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedAddUrls(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::cloudcrawler::AddRequest,::cloudcrawler::Empty>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_GetUrl<WithStreamedUnaryMethod_AddUrls<Service > > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_GetUrl<WithStreamedUnaryMethod_AddUrls<Service > > StreamedService;
};

}  // namespace cloudcrawler


#include <grpcpp/ports_undef.inc>
#endif  // GRPC_info_2eproto__INCLUDED

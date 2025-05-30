// Generated by the protocol buffer compiler.  DO NOT EDIT!
// NO CHECKED-IN PROTOBUF GENCODE
// source: info.proto
// Protobuf C++ Version: 5.29.3

#include "info.pb.h"

#include <algorithm>
#include <type_traits>
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/wire_format_lite.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/wire_format.h"
// @@protoc_insertion_point(includes)

// Must be included last.
#include "google/protobuf/port_def.inc"
PROTOBUF_PRAGMA_INIT_SEG
namespace _pb = ::google::protobuf;
namespace _pbi = ::google::protobuf::internal;
namespace _fl = ::google::protobuf::internal::field_layout;
namespace cloudcrawler {

inline constexpr GetResponse::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : url_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        _cached_size_{0} {}

template <typename>
PROTOBUF_CONSTEXPR GetResponse::GetResponse(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(_class_data_.base()),
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(),
#endif  // PROTOBUF_CUSTOM_VTABLE
      _impl_(::_pbi::ConstantInitialized()) {
}
struct GetResponseDefaultTypeInternal {
  PROTOBUF_CONSTEXPR GetResponseDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~GetResponseDefaultTypeInternal() {}
  union {
    GetResponse _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 GetResponseDefaultTypeInternal _GetResponse_default_instance_;
              template <typename>
PROTOBUF_CONSTEXPR Empty::Empty(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::internal::ZeroFieldsBase(_class_data_.base()){}
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::internal::ZeroFieldsBase() {
}
#endif  // PROTOBUF_CUSTOM_VTABLE
struct EmptyDefaultTypeInternal {
  PROTOBUF_CONSTEXPR EmptyDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~EmptyDefaultTypeInternal() {}
  union {
    Empty _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 EmptyDefaultTypeInternal _Empty_default_instance_;

inline constexpr AddRequest::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : url_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        rank_{0u},
        _cached_size_{0} {}

template <typename>
PROTOBUF_CONSTEXPR AddRequest::AddRequest(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(_class_data_.base()),
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(),
#endif  // PROTOBUF_CUSTOM_VTABLE
      _impl_(::_pbi::ConstantInitialized()) {
}
struct AddRequestDefaultTypeInternal {
  PROTOBUF_CONSTEXPR AddRequestDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~AddRequestDefaultTypeInternal() {}
  union {
    AddRequest _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 AddRequestDefaultTypeInternal _AddRequest_default_instance_;
}  // namespace cloudcrawler
static constexpr const ::_pb::EnumDescriptor**
    file_level_enum_descriptors_info_2eproto = nullptr;
static constexpr const ::_pb::ServiceDescriptor**
    file_level_service_descriptors_info_2eproto = nullptr;
const ::uint32_t
    TableStruct_info_2eproto::offsets[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
        protodesc_cold) = {
        ~0u,  // no _has_bits_
        PROTOBUF_FIELD_OFFSET(::cloudcrawler::Empty, _internal_metadata_),
        ~0u,  // no _extensions_
        ~0u,  // no _oneof_case_
        ~0u,  // no _weak_field_map_
        ~0u,  // no _inlined_string_donated_
        ~0u,  // no _split_
        ~0u,  // no sizeof(Split)
        ~0u,  // no _has_bits_
        PROTOBUF_FIELD_OFFSET(::cloudcrawler::GetResponse, _internal_metadata_),
        ~0u,  // no _extensions_
        ~0u,  // no _oneof_case_
        ~0u,  // no _weak_field_map_
        ~0u,  // no _inlined_string_donated_
        ~0u,  // no _split_
        ~0u,  // no sizeof(Split)
        PROTOBUF_FIELD_OFFSET(::cloudcrawler::GetResponse, _impl_.url_),
        ~0u,  // no _has_bits_
        PROTOBUF_FIELD_OFFSET(::cloudcrawler::AddRequest, _internal_metadata_),
        ~0u,  // no _extensions_
        ~0u,  // no _oneof_case_
        ~0u,  // no _weak_field_map_
        ~0u,  // no _inlined_string_donated_
        ~0u,  // no _split_
        ~0u,  // no sizeof(Split)
        PROTOBUF_FIELD_OFFSET(::cloudcrawler::AddRequest, _impl_.url_),
        PROTOBUF_FIELD_OFFSET(::cloudcrawler::AddRequest, _impl_.rank_),
};

static const ::_pbi::MigrationSchema
    schemas[] ABSL_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
        {0, -1, -1, sizeof(::cloudcrawler::Empty)},
        {8, -1, -1, sizeof(::cloudcrawler::GetResponse)},
        {17, -1, -1, sizeof(::cloudcrawler::AddRequest)},
};
static const ::_pb::Message* const file_default_instances[] = {
    &::cloudcrawler::_Empty_default_instance_._instance,
    &::cloudcrawler::_GetResponse_default_instance_._instance,
    &::cloudcrawler::_AddRequest_default_instance_._instance,
};
const char descriptor_table_protodef_info_2eproto[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
    protodesc_cold) = {
    "\n\ninfo.proto\022\014cloudcrawler\"\007\n\005Empty\"\032\n\013G"
    "etResponse\022\013\n\003url\030\001 \001(\t\"\'\n\nAddRequest\022\013\n"
    "\003url\030\001 \001(\t\022\014\n\004rank\030\002 \001(\r2\204\001\n\nDispatcher\022"
    ":\n\006GetUrl\022\023.cloudcrawler.Empty\032\031.cloudcr"
    "awler.GetResponse\"\000\022:\n\007AddUrls\022\030.cloudcr"
    "awler.AddRequest\032\023.cloudcrawler.Empty\"\000b"
    "\006proto3"
};
static ::absl::once_flag descriptor_table_info_2eproto_once;
PROTOBUF_CONSTINIT const ::_pbi::DescriptorTable descriptor_table_info_2eproto = {
    false,
    false,
    247,
    descriptor_table_protodef_info_2eproto,
    "info.proto",
    &descriptor_table_info_2eproto_once,
    nullptr,
    0,
    3,
    schemas,
    file_default_instances,
    TableStruct_info_2eproto::offsets,
    file_level_enum_descriptors_info_2eproto,
    file_level_service_descriptors_info_2eproto,
};
namespace cloudcrawler {
// ===================================================================

class Empty::_Internal {
 public:
};

Empty::Empty(::google::protobuf::Arena* arena)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::internal::ZeroFieldsBase(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::internal::ZeroFieldsBase(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  // @@protoc_insertion_point(arena_constructor:cloudcrawler.Empty)
}
Empty::Empty(
    ::google::protobuf::Arena* arena,
    const Empty& from)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::internal::ZeroFieldsBase(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::internal::ZeroFieldsBase(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  Empty* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);

  // @@protoc_insertion_point(copy_constructor:cloudcrawler.Empty)
}

inline void* Empty::PlacementNew_(const void*, void* mem,
                                        ::google::protobuf::Arena* arena) {
  return ::new (mem) Empty(arena);
}
constexpr auto Empty::InternalNewImpl_() {
  return ::google::protobuf::internal::MessageCreator::ZeroInit(sizeof(Empty),
                                            alignof(Empty));
}
PROTOBUF_CONSTINIT
PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::google::protobuf::internal::ClassDataFull Empty::_class_data_ = {
    ::google::protobuf::internal::ClassData{
        &_Empty_default_instance_._instance,
        &_table_.header,
        nullptr,  // OnDemandRegisterArenaDtor
        nullptr,  // IsInitialized
        &Empty::MergeImpl,
        ::google::protobuf::internal::ZeroFieldsBase::GetNewImpl<Empty>(),
#if defined(PROTOBUF_CUSTOM_VTABLE)
        &Empty::SharedDtor,
        ::google::protobuf::internal::ZeroFieldsBase::GetClearImpl<Empty>(), &Empty::ByteSizeLong,
            &Empty::_InternalSerialize,
#endif  // PROTOBUF_CUSTOM_VTABLE
        PROTOBUF_FIELD_OFFSET(Empty, _impl_._cached_size_),
        false,
    },
    &Empty::kDescriptorMethods,
    &descriptor_table_info_2eproto,
    nullptr,  // tracker
};
const ::google::protobuf::internal::ClassData* Empty::GetClassData() const {
  ::google::protobuf::internal::PrefetchToLocalCache(&_class_data_);
  ::google::protobuf::internal::PrefetchToLocalCache(_class_data_.tc_table);
  return _class_data_.base();
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<0, 0, 0, 0, 2> Empty::_table_ = {
  {
    0,  // no _has_bits_
    0, // no _extensions_
    0, 0,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967295,  // skipmap
    offsetof(decltype(_table_), field_names),  // no field_entries
    0,  // num_field_entries
    0,  // num_aux_entries
    offsetof(decltype(_table_), field_names),  // no aux_entries
    _class_data_.base(),
    nullptr,  // post_loop_handler
    ::_pbi::TcParser::GenericFallback,  // fallback
    #ifdef PROTOBUF_PREFETCH_PARSE_TABLE
    ::_pbi::TcParser::GetTable<::cloudcrawler::Empty>(),  // to_prefetch
    #endif  // PROTOBUF_PREFETCH_PARSE_TABLE
  }, {{
    {::_pbi::TcParser::MiniParse, {}},
  }}, {{
    65535, 65535
  }},
  // no field_entries, or aux_entries
  {{
  }},
};








::google::protobuf::Metadata Empty::GetMetadata() const {
  return ::google::protobuf::internal::ZeroFieldsBase::GetMetadataImpl(GetClassData()->full());
}
// ===================================================================

class GetResponse::_Internal {
 public:
};

GetResponse::GetResponse(::google::protobuf::Arena* arena)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:cloudcrawler.GetResponse)
}
inline PROTOBUF_NDEBUG_INLINE GetResponse::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from, const ::cloudcrawler::GetResponse& from_msg)
      : url_(arena, from.url_),
        _cached_size_{0} {}

GetResponse::GetResponse(
    ::google::protobuf::Arena* arena,
    const GetResponse& from)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  GetResponse* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_, from);

  // @@protoc_insertion_point(copy_constructor:cloudcrawler.GetResponse)
}
inline PROTOBUF_NDEBUG_INLINE GetResponse::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : url_(arena),
        _cached_size_{0} {}

inline void GetResponse::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
}
GetResponse::~GetResponse() {
  // @@protoc_insertion_point(destructor:cloudcrawler.GetResponse)
  SharedDtor(*this);
}
inline void GetResponse::SharedDtor(MessageLite& self) {
  GetResponse& this_ = static_cast<GetResponse&>(self);
  this_._internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  ABSL_DCHECK(this_.GetArena() == nullptr);
  this_._impl_.url_.Destroy();
  this_._impl_.~Impl_();
}

inline void* GetResponse::PlacementNew_(const void*, void* mem,
                                        ::google::protobuf::Arena* arena) {
  return ::new (mem) GetResponse(arena);
}
constexpr auto GetResponse::InternalNewImpl_() {
  return ::google::protobuf::internal::MessageCreator::CopyInit(sizeof(GetResponse),
                                            alignof(GetResponse));
}
PROTOBUF_CONSTINIT
PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::google::protobuf::internal::ClassDataFull GetResponse::_class_data_ = {
    ::google::protobuf::internal::ClassData{
        &_GetResponse_default_instance_._instance,
        &_table_.header,
        nullptr,  // OnDemandRegisterArenaDtor
        nullptr,  // IsInitialized
        &GetResponse::MergeImpl,
        ::google::protobuf::Message::GetNewImpl<GetResponse>(),
#if defined(PROTOBUF_CUSTOM_VTABLE)
        &GetResponse::SharedDtor,
        ::google::protobuf::Message::GetClearImpl<GetResponse>(), &GetResponse::ByteSizeLong,
            &GetResponse::_InternalSerialize,
#endif  // PROTOBUF_CUSTOM_VTABLE
        PROTOBUF_FIELD_OFFSET(GetResponse, _impl_._cached_size_),
        false,
    },
    &GetResponse::kDescriptorMethods,
    &descriptor_table_info_2eproto,
    nullptr,  // tracker
};
const ::google::protobuf::internal::ClassData* GetResponse::GetClassData() const {
  ::google::protobuf::internal::PrefetchToLocalCache(&_class_data_);
  ::google::protobuf::internal::PrefetchToLocalCache(_class_data_.tc_table);
  return _class_data_.base();
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<0, 1, 0, 36, 2> GetResponse::_table_ = {
  {
    0,  // no _has_bits_
    0, // no _extensions_
    1, 0,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967294,  // skipmap
    offsetof(decltype(_table_), field_entries),
    1,  // num_field_entries
    0,  // num_aux_entries
    offsetof(decltype(_table_), field_names),  // no aux_entries
    _class_data_.base(),
    nullptr,  // post_loop_handler
    ::_pbi::TcParser::GenericFallback,  // fallback
    #ifdef PROTOBUF_PREFETCH_PARSE_TABLE
    ::_pbi::TcParser::GetTable<::cloudcrawler::GetResponse>(),  // to_prefetch
    #endif  // PROTOBUF_PREFETCH_PARSE_TABLE
  }, {{
    // string url = 1;
    {::_pbi::TcParser::FastUS1,
     {10, 63, 0, PROTOBUF_FIELD_OFFSET(GetResponse, _impl_.url_)}},
  }}, {{
    65535, 65535
  }}, {{
    // string url = 1;
    {PROTOBUF_FIELD_OFFSET(GetResponse, _impl_.url_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
  }},
  // no aux_entries
  {{
    "\30\3\0\0\0\0\0\0"
    "cloudcrawler.GetResponse"
    "url"
  }},
};

PROTOBUF_NOINLINE void GetResponse::Clear() {
// @@protoc_insertion_point(message_clear_start:cloudcrawler.GetResponse)
  ::google::protobuf::internal::TSanWrite(&_impl_);
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.url_.ClearToEmpty();
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::uint8_t* GetResponse::_InternalSerialize(
            const MessageLite& base, ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) {
          const GetResponse& this_ = static_cast<const GetResponse&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::uint8_t* GetResponse::_InternalSerialize(
            ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) const {
          const GetResponse& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(serialize_to_array_start:cloudcrawler.GetResponse)
          ::uint32_t cached_has_bits = 0;
          (void)cached_has_bits;

          // string url = 1;
          if (!this_._internal_url().empty()) {
            const std::string& _s = this_._internal_url();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "cloudcrawler.GetResponse.url");
            target = stream->WriteStringMaybeAliased(1, _s, target);
          }

          if (PROTOBUF_PREDICT_FALSE(this_._internal_metadata_.have_unknown_fields())) {
            target =
                ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
                    this_._internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
          }
          // @@protoc_insertion_point(serialize_to_array_end:cloudcrawler.GetResponse)
          return target;
        }

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::size_t GetResponse::ByteSizeLong(const MessageLite& base) {
          const GetResponse& this_ = static_cast<const GetResponse&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::size_t GetResponse::ByteSizeLong() const {
          const GetResponse& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(message_byte_size_start:cloudcrawler.GetResponse)
          ::size_t total_size = 0;

          ::uint32_t cached_has_bits = 0;
          // Prevent compiler warnings about cached_has_bits being unused
          (void)cached_has_bits;

           {
            // string url = 1;
            if (!this_._internal_url().empty()) {
              total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                              this_._internal_url());
            }
          }
          return this_.MaybeComputeUnknownFieldsSize(total_size,
                                                     &this_._impl_._cached_size_);
        }

void GetResponse::MergeImpl(::google::protobuf::MessageLite& to_msg, const ::google::protobuf::MessageLite& from_msg) {
  auto* const _this = static_cast<GetResponse*>(&to_msg);
  auto& from = static_cast<const GetResponse&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:cloudcrawler.GetResponse)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_url().empty()) {
    _this->_internal_set_url(from._internal_url());
  }
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}

void GetResponse::CopyFrom(const GetResponse& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:cloudcrawler.GetResponse)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}


void GetResponse::InternalSwap(GetResponse* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.url_, &other->_impl_.url_, arena);
}

::google::protobuf::Metadata GetResponse::GetMetadata() const {
  return ::google::protobuf::Message::GetMetadataImpl(GetClassData()->full());
}
// ===================================================================

class AddRequest::_Internal {
 public:
};

AddRequest::AddRequest(::google::protobuf::Arena* arena)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:cloudcrawler.AddRequest)
}
inline PROTOBUF_NDEBUG_INLINE AddRequest::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from, const ::cloudcrawler::AddRequest& from_msg)
      : url_(arena, from.url_),
        _cached_size_{0} {}

AddRequest::AddRequest(
    ::google::protobuf::Arena* arena,
    const AddRequest& from)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  AddRequest* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_, from);
  _impl_.rank_ = from._impl_.rank_;

  // @@protoc_insertion_point(copy_constructor:cloudcrawler.AddRequest)
}
inline PROTOBUF_NDEBUG_INLINE AddRequest::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : url_(arena),
        _cached_size_{0} {}

inline void AddRequest::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  _impl_.rank_ = {};
}
AddRequest::~AddRequest() {
  // @@protoc_insertion_point(destructor:cloudcrawler.AddRequest)
  SharedDtor(*this);
}
inline void AddRequest::SharedDtor(MessageLite& self) {
  AddRequest& this_ = static_cast<AddRequest&>(self);
  this_._internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  ABSL_DCHECK(this_.GetArena() == nullptr);
  this_._impl_.url_.Destroy();
  this_._impl_.~Impl_();
}

inline void* AddRequest::PlacementNew_(const void*, void* mem,
                                        ::google::protobuf::Arena* arena) {
  return ::new (mem) AddRequest(arena);
}
constexpr auto AddRequest::InternalNewImpl_() {
  return ::google::protobuf::internal::MessageCreator::CopyInit(sizeof(AddRequest),
                                            alignof(AddRequest));
}
PROTOBUF_CONSTINIT
PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::google::protobuf::internal::ClassDataFull AddRequest::_class_data_ = {
    ::google::protobuf::internal::ClassData{
        &_AddRequest_default_instance_._instance,
        &_table_.header,
        nullptr,  // OnDemandRegisterArenaDtor
        nullptr,  // IsInitialized
        &AddRequest::MergeImpl,
        ::google::protobuf::Message::GetNewImpl<AddRequest>(),
#if defined(PROTOBUF_CUSTOM_VTABLE)
        &AddRequest::SharedDtor,
        ::google::protobuf::Message::GetClearImpl<AddRequest>(), &AddRequest::ByteSizeLong,
            &AddRequest::_InternalSerialize,
#endif  // PROTOBUF_CUSTOM_VTABLE
        PROTOBUF_FIELD_OFFSET(AddRequest, _impl_._cached_size_),
        false,
    },
    &AddRequest::kDescriptorMethods,
    &descriptor_table_info_2eproto,
    nullptr,  // tracker
};
const ::google::protobuf::internal::ClassData* AddRequest::GetClassData() const {
  ::google::protobuf::internal::PrefetchToLocalCache(&_class_data_);
  ::google::protobuf::internal::PrefetchToLocalCache(_class_data_.tc_table);
  return _class_data_.base();
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<1, 2, 0, 35, 2> AddRequest::_table_ = {
  {
    0,  // no _has_bits_
    0, // no _extensions_
    2, 8,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967292,  // skipmap
    offsetof(decltype(_table_), field_entries),
    2,  // num_field_entries
    0,  // num_aux_entries
    offsetof(decltype(_table_), field_names),  // no aux_entries
    _class_data_.base(),
    nullptr,  // post_loop_handler
    ::_pbi::TcParser::GenericFallback,  // fallback
    #ifdef PROTOBUF_PREFETCH_PARSE_TABLE
    ::_pbi::TcParser::GetTable<::cloudcrawler::AddRequest>(),  // to_prefetch
    #endif  // PROTOBUF_PREFETCH_PARSE_TABLE
  }, {{
    // uint32 rank = 2;
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(AddRequest, _impl_.rank_), 63>(),
     {16, 63, 0, PROTOBUF_FIELD_OFFSET(AddRequest, _impl_.rank_)}},
    // string url = 1;
    {::_pbi::TcParser::FastUS1,
     {10, 63, 0, PROTOBUF_FIELD_OFFSET(AddRequest, _impl_.url_)}},
  }}, {{
    65535, 65535
  }}, {{
    // string url = 1;
    {PROTOBUF_FIELD_OFFSET(AddRequest, _impl_.url_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // uint32 rank = 2;
    {PROTOBUF_FIELD_OFFSET(AddRequest, _impl_.rank_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUInt32)},
  }},
  // no aux_entries
  {{
    "\27\3\0\0\0\0\0\0"
    "cloudcrawler.AddRequest"
    "url"
  }},
};

PROTOBUF_NOINLINE void AddRequest::Clear() {
// @@protoc_insertion_point(message_clear_start:cloudcrawler.AddRequest)
  ::google::protobuf::internal::TSanWrite(&_impl_);
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.url_.ClearToEmpty();
  _impl_.rank_ = 0u;
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::uint8_t* AddRequest::_InternalSerialize(
            const MessageLite& base, ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) {
          const AddRequest& this_ = static_cast<const AddRequest&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::uint8_t* AddRequest::_InternalSerialize(
            ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) const {
          const AddRequest& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(serialize_to_array_start:cloudcrawler.AddRequest)
          ::uint32_t cached_has_bits = 0;
          (void)cached_has_bits;

          // string url = 1;
          if (!this_._internal_url().empty()) {
            const std::string& _s = this_._internal_url();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "cloudcrawler.AddRequest.url");
            target = stream->WriteStringMaybeAliased(1, _s, target);
          }

          // uint32 rank = 2;
          if (this_._internal_rank() != 0) {
            target = stream->EnsureSpace(target);
            target = ::_pbi::WireFormatLite::WriteUInt32ToArray(
                2, this_._internal_rank(), target);
          }

          if (PROTOBUF_PREDICT_FALSE(this_._internal_metadata_.have_unknown_fields())) {
            target =
                ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
                    this_._internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
          }
          // @@protoc_insertion_point(serialize_to_array_end:cloudcrawler.AddRequest)
          return target;
        }

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::size_t AddRequest::ByteSizeLong(const MessageLite& base) {
          const AddRequest& this_ = static_cast<const AddRequest&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::size_t AddRequest::ByteSizeLong() const {
          const AddRequest& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(message_byte_size_start:cloudcrawler.AddRequest)
          ::size_t total_size = 0;

          ::uint32_t cached_has_bits = 0;
          // Prevent compiler warnings about cached_has_bits being unused
          (void)cached_has_bits;

          ::_pbi::Prefetch5LinesFrom7Lines(&this_);
           {
            // string url = 1;
            if (!this_._internal_url().empty()) {
              total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                              this_._internal_url());
            }
            // uint32 rank = 2;
            if (this_._internal_rank() != 0) {
              total_size += ::_pbi::WireFormatLite::UInt32SizePlusOne(
                  this_._internal_rank());
            }
          }
          return this_.MaybeComputeUnknownFieldsSize(total_size,
                                                     &this_._impl_._cached_size_);
        }

void AddRequest::MergeImpl(::google::protobuf::MessageLite& to_msg, const ::google::protobuf::MessageLite& from_msg) {
  auto* const _this = static_cast<AddRequest*>(&to_msg);
  auto& from = static_cast<const AddRequest&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:cloudcrawler.AddRequest)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_url().empty()) {
    _this->_internal_set_url(from._internal_url());
  }
  if (from._internal_rank() != 0) {
    _this->_impl_.rank_ = from._impl_.rank_;
  }
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}

void AddRequest::CopyFrom(const AddRequest& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:cloudcrawler.AddRequest)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}


void AddRequest::InternalSwap(AddRequest* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.url_, &other->_impl_.url_, arena);
        swap(_impl_.rank_, other->_impl_.rank_);
}

::google::protobuf::Metadata AddRequest::GetMetadata() const {
  return ::google::protobuf::Message::GetMetadataImpl(GetClassData()->full());
}
// @@protoc_insertion_point(namespace_scope)
}  // namespace cloudcrawler
namespace google {
namespace protobuf {
}  // namespace protobuf
}  // namespace google
// @@protoc_insertion_point(global_scope)
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::std::false_type
    _static_init2_ PROTOBUF_UNUSED =
        (::_pbi::AddDescriptors(&descriptor_table_info_2eproto),
         ::std::false_type{});
#include "google/protobuf/port_undef.inc"

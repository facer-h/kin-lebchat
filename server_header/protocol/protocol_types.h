#ifndef KINCHAT_HEADER_PROTOCOL_PROTOCOL_TYPES_H_
#define KINCHAT_HEADER_PROTOCOL_PROTOCOL_TYPES_H_

#include <cstdint>
#include <string>

#include "protocol/protocol_constants.h"

// 客户端与服务器之间的消息类型。
enum class MessageType : std::uint16_t {
  // 0-99：系统级。
  kUnknown = 0, // 未知类型
  kHeartbeatRequest = 1,  // 心跳请求
  kHeartbeatResponse = 2, // 心跳响应
  kErrorResponse = 3, // 错误响应

  // 100-199：用户相关。
  kRegisterRequest = 100, // 注册请求
  kRegisterResponse = 101, // 注册响应
  kLoginRequest = 102, // 登录请求
  kLoginResponse = 103, // 登录响应
  kLogoutRequest = 104, // 登出请求
  kLogoutResponse = 105, // 登出响应
  kUserInfoRequest = 106, // 用户信息请求
  kUserInfoResponse = 107, // 用户信息响应
  kUserSearchRequest = 108, // 用户搜索请求
  kUserSearchResponse = 109, // 用户搜索响应

  // 200-299：好友相关。
  kFriendAddRequest = 200,
  kFriendAddResponse = 201,
  kFriendReplyRequest = 202,
  kFriendReplyResponse = 203,
  kFriendDeleteRequest = 204,
  kFriendDeleteResponse = 205,
  kFriendListRequest = 206,
  kFriendListResponse = 207,
  kFriendRequestPush = 208,
  kFriendStatusPush = 209,

  // 300-399：会话与群成员相关。
  kConversationCreateRequest = 300,
  kConversationCreateResponse = 301,
  kConversationListRequest = 302,
  kConversationListResponse = 303,
  kConversationMemberRequest = 304,
  kConversationMemberResponse = 305,
  kConversationUpdateRequest = 306,
  kConversationUpdateResponse = 307,
  kConversationLeaveRequest = 308,
  kConversationLeaveResponse = 309,
  kConversationMemberAddRequest = 310,
  kConversationMemberAddResponse = 311,
  kConversationMemberRemoveRequest = 312,
  kConversationMemberRemoveResponse = 313,

  // 400-499：消息相关。
  kMessageSendRequest = 400,
  kMessageSendResponse = 401,
  kMessagePush = 402,
  kMessageHistoryRequest = 403,
  kMessageHistoryResponse = 404,
  kMessageAckRequest = 405,
  kMessageAckResponse = 406,
};

// 协议包标志位，可以通过按位或组合。
enum PacketFlag : std::uint16_t {
  kFlagNone = 0,
  kFlagResponse = 1 << 0,
  kFlagServerPush = 1 << 1,
  kFlagCompressed = 1 << 2,
  kFlagEncrypted = 1 << 3,
};

// 服务器返回的统一业务状态码。
enum class StatusCode : std::uint16_t {
  kSuccess = 0,
  kInvalidPacket = 1,
  kUnsupportedVersion = 2,
  kUnsupportedMessageType = 3,
  kInvalidParameter = 4,
  kNotAuthenticated = 5,
  kPermissionDenied = 6,
  kUserNotFound = 7,
  kAccountAlreadyExists = 8,
  kPasswordIncorrect = 9,
  kFriendRelationAlreadyExists = 10,
  kConversationNotFound = 11,
  kNotConversationMember = 12,
  kMessageNotFound = 13,
  kDatabaseError = 14,
  kServerBusy = 15,
  kInternalError = 16,
};

// 固定长度为 24 字节的逻辑包头。
// 禁止直接 memcpy 该结构体进行网络传输，必须逐字段编码以避免内存对齐问题。
struct PacketHeader {
  std::uint32_t magic = kProtocolMagic;
  std::uint16_t version = kProtocolVersion;
  std::uint16_t header_size = kProtocolHeaderSize;
  MessageType message_type = MessageType::kUnknown;
  std::uint16_t flags = kFlagNone;
  std::uint32_t body_size = 0;
  std::uint64_t sequence_id = 0;
};

// 一个经过解码的完整协议包，body 保存 UTF-8 JSON 文本。
struct ProtocolPacket {
  PacketHeader header;
  std::string body;
};

enum class DecodeResult {
  kSuccess,
  kNeedMoreData,
  kInvalidPacket,
};

#endif  // KINCHAT_HEADER_PROTOCOL_PROTOCOL_TYPES_H_

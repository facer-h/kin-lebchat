#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// 协议中的所有整数在线上传输时统一使用网络字节序（大端序）。
inline constexpr std::uint32_t kProtocolMagic = 0x43484154;  // ASCII: CHAT
inline constexpr std::uint16_t kProtocolVersion = 1;
inline constexpr std::uint16_t kProtocolHeaderSize = 24;
inline constexpr std::uint32_t kMaxProtocolBodySize = 1024 * 1024;

// 客户端与服务器之间的消息类型。
enum class MessageType : std::uint16_t {
  kUnknown = 0,

  kHeartbeatRequest = 1,
  kHeartbeatResponse = 2,
  kErrorResponse = 3,

  kRegisterRequest = 100,
  kRegisterResponse = 101,
  kLoginRequest = 102,
  kLoginResponse = 103,
  kLogoutRequest = 104,
  kLogoutResponse = 105,
  kUserInfoRequest = 106,
  kUserInfoResponse = 107,

  kFriendAddRequest = 200,
  kFriendAddResponse = 201,
  kFriendReplyRequest = 202,
  kFriendReplyResponse = 203,
  kFriendDeleteRequest = 204,
  kFriendDeleteResponse = 205,
  kFriendListRequest = 206,
  kFriendListResponse = 207,
  kFriendRequestPush = 208,

  kConversationCreateRequest = 300,
  kConversationCreateResponse = 301,
  kConversationListRequest = 302,
  kConversationListResponse = 303,
  kConversationMemberRequest = 304,
  kConversationMemberResponse = 305,

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

class ProtocolCodec {
 public:
  /*
   * 参数：packet，需要编码的协议包；output，接收编码结果的字节数组。
   * 功能：将包头转换为网络字节序，并在其后追加 UTF-8 JSON 包体。
   */
  static bool Encode(const ProtocolPacket& packet,
                     std::vector<std::uint8_t>* output);

  /*
   * 参数：buffer，客户端累计接收缓冲区；packet，接收解码结果；
   *       error，接收协议错误信息，可以为空。
   * 功能：从 TCP 字节流中解码一个完整协议包；成功时从 buffer 移除该包，
   *       数据不足时保留 buffer，非法包时返回 kInvalidPacket。
   */
  static DecodeResult DecodeOne(std::vector<std::uint8_t>* buffer,
                                ProtocolPacket* packet, std::string* error);

  /*
   * 参数：header，需要校验的包头；error，接收错误信息，可以为空。
   * 功能：检查魔数、协议版本、包头长度、消息类型和包体长度是否合法。
   */
  static bool ValidateHeader(const PacketHeader& header, std::string* error);
};

#endif  // PROTOCOL_H_

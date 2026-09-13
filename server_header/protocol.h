#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// 协议中的所有整数在线上传输时统一使用网络字节序（大端序）。
inline constexpr std::uint32_t kProtocolMagic = 0x43484154;  // ASCII: CHAT 协议标识
inline constexpr std::uint16_t kProtocolVersion = 1;         /* 协议版本 */
inline constexpr std::uint16_t kProtocolHeaderSize = 24;    /* 协议头大小 */
inline constexpr std::uint32_t kMaxProtocolBodySize = 1024 * 1024;  /* 最大协议体大小  1m*/

// 客户端与服务器之间的消息类型。
enum class MessageType : std::uint16_t {

  /*  0-99 系统级 */
  kUnknown = 0, /* 未知消息类型 */
  kHeartbeatRequest = 1, /* 心跳请求 */
  kHeartbeatResponse = 2, /* 心跳响应 */
  kErrorResponse = 3, /* 错误响应 */

  /*  100-199 用户相关 */
  kRegisterRequest = 100, /* 注册请求 */
  kRegisterResponse = 101, /* 注册响应 */
  kLoginRequest = 102, /* 登录请求 */
  kLoginResponse = 103, /* 登录响应 */
  kLogoutRequest = 104, /* 登出请求 */
  kLogoutResponse = 105, /* 登出响应 */
  kUserInfoRequest = 106, /* 用户信息请求 */
  kUserInfoResponse = 107, /* 用户信息响应 */

  /*  200-299 好友相关 */
  kFriendAddRequest = 200, /* 添加好友请求 */
  kFriendAddResponse = 201, /* 添加好友响应 */
  kFriendReplyRequest = 202, /* 好友回复请求 */
  kFriendReplyResponse = 203, /* 好友回复响应 */
  kFriendDeleteRequest = 204, /* 删除好友请求 */
  kFriendDeleteResponse = 205, /* 删除好友响应 */
  kFriendListRequest = 206, /* 好友列表请求 */
  kFriendListResponse = 207,    /* 好友列表响应 */
  kFriendRequestPush = 208,   /* 好友请求推送 */

  /*  300-399 会话相关 */
  kConversationCreateRequest = 300, /* 创建会话请求 */
  kConversationCreateResponse = 301, /* 创建会话响应 */
  kConversationListRequest = 302, /* 会话列表请求 */
  kConversationListResponse = 303, /* 会话列表响应 */
  kConversationMemberRequest = 304, /* 会话成员请求 */
  kConversationMemberResponse = 305, /* 会话成员响应 */

  /*  400-499 消息相关 */
  kMessageSendRequest = 400, /* 发送消息请求 */
  kMessageSendResponse = 401, /* 发送消息响应 */
  kMessagePush = 402, /* 消息推送 */
  kMessageHistoryRequest = 403, /* 消息历史请求 */
  kMessageHistoryResponse = 404, /* 消息历史响应 */
  kMessageAckRequest = 405, /* 消息确认请求 */
  kMessageAckResponse = 406, /* 消息确认响应 */
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
  kSuccess = 0, // 成功
  kInvalidPacket = 1, // 非法协议包
  kUnsupportedVersion = 2,  // 不支持的协议版本
  kUnsupportedMessageType = 3, // 不支持的消息类型
  kInvalidParameter = 4, // 无效参数
  kNotAuthenticated = 5, // 未认证
  kPermissionDenied = 6, // 权限被拒绝
  kUserNotFound = 7, // 用户未找到
  kAccountAlreadyExists = 8, // 账户已存在
  kPasswordIncorrect = 9, // 密码错误
  kFriendRelationAlreadyExists = 10, // 好友关系已存在
  kConversationNotFound = 11, // 会话未找到
  kNotConversationMember = 12, // 不是会话成员
  kMessageNotFound = 13, // 消息未找到
  kDatabaseError = 14, // 数据库错误
  kServerBusy = 15, // 服务器繁忙
  kInternalError = 16, // 内部错误
};

// 固定长度为 24 字节的逻辑包头。
// 禁止直接 memcpy 该结构体进行网络传输，必须逐字段编码以避免内存对齐问题。
struct PacketHeader {
  std::uint32_t magic = kProtocolMagic;   // 协议魔数
  std::uint16_t version = kProtocolVersion; // 协议版本
  std::uint16_t header_size = kProtocolHeaderSize; // 包头大小
  MessageType message_type = MessageType::kUnknown; // 消息类型
  std::uint16_t flags = kFlagNone; // 标志位
  std::uint32_t body_size = 0; // 包体大小
  std::uint64_t sequence_id = 0; // 序列号
};

// 一个经过解码的完整协议包，body 保存 UTF-8 JSON 文本。
struct ProtocolPacket {
  PacketHeader header;  // 包头
  std::string body; // 包体
};

enum class DecodeResult {
  kSuccess, // 解码成功
  kNeedMoreData, // 需要更多数据
  kInvalidPacket, // 非法协议包
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

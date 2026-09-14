#ifndef KINCHAT_HEADER_DATABASE_DATABASE_SCHEMA_H_
#define KINCHAT_HEADER_DATABASE_DATABASE_SCHEMA_H_

#include <cstdint>

// 数据库表名、字段名和状态值的唯一 C++ 定义。
// 完整建表结构以 database/migrations/001_initial_schema.sql 为准。
namespace db {

inline constexpr char kDatabaseName[] = "User";

namespace user {
inline constexpr char kTable[] = "User";
inline constexpr char kUserId[] = "UserId";
inline constexpr char kUserAccount[] = "UserAccount";
inline constexpr char kUserPassword[] = "UserPassword";
inline constexpr char kNickname[] = "Nickname";
inline constexpr char kAvatarUrl[] = "AvatarUrl";
inline constexpr char kCreatedAt[] = "CreatedAt";
inline constexpr char kUpdatedAt[] = "UpdatedAt";
inline constexpr char kStatus[] = "Status";
}  // namespace user

namespace user_friend {
inline constexpr char kTable[] = "UserFriend";
inline constexpr char kId[] = "Id";
inline constexpr char kUserId[] = "UserId";
inline constexpr char kFriendId[] = "FriendId";
inline constexpr char kStatus[] = "Status";
inline constexpr char kRemark[] = "Remark";
inline constexpr char kCreatedAt[] = "CreatedAt";
inline constexpr char kUpdatedAt[] = "UpdatedAt";
}  // namespace user_friend

namespace conversation {
inline constexpr char kTable[] = "Conversation";
inline constexpr char kConversationId[] = "ConversationId";
inline constexpr char kConversationType[] = "ConversationType";
inline constexpr char kConversationName[] = "ConversationName";
inline constexpr char kOwnerId[] = "OwnerId";
inline constexpr char kPrivateUserLowId[] = "PrivateUserLowId";
inline constexpr char kPrivateUserHighId[] = "PrivateUserHighId";
inline constexpr char kAvatarUrl[] = "AvatarUrl";
inline constexpr char kStatus[] = "Status";
inline constexpr char kCreatedAt[] = "CreatedAt";
inline constexpr char kUpdatedAt[] = "UpdatedAt";
}  // namespace conversation

namespace conversation_member {
inline constexpr char kTable[] = "ConversationMember";
inline constexpr char kConversationId[] = "ConversationId";
inline constexpr char kUserId[] = "UserId";
inline constexpr char kRole[] = "Role";
inline constexpr char kMuteStatus[] = "MuteStatus";
inline constexpr char kLastReadMessageId[] = "LastReadMessageId";
inline constexpr char kJoinedAt[] = "JoinedAt";
inline constexpr char kLeftAt[] = "LeftAt";
}  // namespace conversation_member

namespace message {
inline constexpr char kTable[] = "Message";
inline constexpr char kMessageId[] = "MessageId";
inline constexpr char kConversationId[] = "ConversationId";
inline constexpr char kSenderId[] = "SenderId";
inline constexpr char kClientMessageId[] = "ClientMessageId";
inline constexpr char kMessageType[] = "MessageType";
inline constexpr char kContent[] = "Content";
inline constexpr char kReplyMessageId[] = "ReplyMessageId";
inline constexpr char kStatus[] = "Status";
inline constexpr char kSendTime[] = "SendTime";
}  // namespace message

enum class UserStatus : std::uint8_t {
  kNormal = 0,
  kFrozen = 1,
  kDeleted = 2,
};

enum class FriendStatus : std::uint8_t {
  kPending = 0,
  kAccepted = 1,
  kRejected = 2,
  kDeleted = 3,
  kBlocked = 4,
};

enum class ConversationType : std::uint8_t {
  kPrivate = 1,
  kGroup = 2,
};

enum class ConversationStatus : std::uint8_t {
  kNormal = 0,
  kDissolved = 1,
  kDisabled = 2,
};

enum class MemberRole : std::uint8_t {
  kMember = 0,
  kAdministrator = 1,
  kOwner = 2,
};

enum class MuteStatus : std::uint8_t {
  kNotMuted = 0,
  kMuted = 1,
};

enum class MessageContentType : std::uint8_t {
  kText = 1,
  kImage = 2,
  kAudio = 3,
  kVideo = 4,
  kFile = 5,
  kSystem = 6,
};

enum class MessageStatus : std::uint8_t {
  kNormal = 0,
  kRecalled = 1,
  kDeleted = 2,
};

}  // namespace db

#endif  // KINCHAT_HEADER_DATABASE_DATABASE_SCHEMA_H_

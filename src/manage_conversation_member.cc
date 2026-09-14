#include "database/manage_conversation_member.h"

#include "database/database_schema.h"

namespace {

// ConversationMember 表允许出现的字段白名单。
const std::vector<std::string>& ConversationMemberColumns() {
  static const std::vector<std::string> kColumns = {
      db::conversation_member::kConversationId,
      db::conversation_member::kUserId,
      db::conversation_member::kRole,
      db::conversation_member::kMuteStatus,
      db::conversation_member::kLastReadMessageId,
      db::conversation_member::kJoinedAt,
      db::conversation_member::kLeftAt,
  };
  return kColumns;
}

// ConversationMember 表的建表语句。
const char* const kCreateTableSql = R"sql(
CREATE TABLE IF NOT EXISTS `ConversationMember` (
  `ConversationId` BIGINT UNSIGNED NOT NULL,
  `UserId` BIGINT UNSIGNED NOT NULL,
  `Role` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `MuteStatus` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `LastReadMessageId` BIGINT UNSIGNED DEFAULT NULL,
  `JoinedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `LeftAt` DATETIME DEFAULT NULL,
  PRIMARY KEY (`ConversationId`, `UserId`),
  KEY `idx_conversation_member_user` (`UserId`, `LeftAt`),
  KEY `idx_conversation_member_last_read` (`LastReadMessageId`),
  CONSTRAINT `fk_conversation_member_conversation`
    FOREIGN KEY (`ConversationId`) REFERENCES `Conversation` (`ConversationId`)
    ON DELETE CASCADE,
  CONSTRAINT `fk_conversation_member_user`
    FOREIGN KEY (`UserId`) REFERENCES `User` (`UserId`) ON DELETE CASCADE,
  CONSTRAINT `chk_conversation_member_role` CHECK (`Role` IN (0, 1, 2)),
  CONSTRAINT `chk_conversation_member_mute` CHECK (`MuteStatus` IN (0, 1))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
)sql";

}  // namespace

bool ManageConversationMember::SqlInit() {
  if (!CreateDatabase()) {
    return false;
  }
  return CreateTable(kCreateTableSql);
}

bool ManageConversationMember::IsRunning() {
  return Ping();
}

bool ManageConversationMember::AddRaw(const RawRow& row) {
  if (!RequireColumns(row, {db::conversation_member::kConversationId,
                            db::conversation_member::kUserId})) {
    return false;
  }
  return ExecuteInsert(db::conversation_member::kTable, ConversationMemberColumns(),
                       row);
}

bool ManageConversationMember::DelRaw(const RawRow& where) {
  return ExecuteDelete(db::conversation_member::kTable, ConversationMemberColumns(),
                       where);
}

bool ManageConversationMember::UpdateRaw(const RawRow& values,
                                         const RawRow& where) {
  return ExecuteUpdate(db::conversation_member::kTable, ConversationMemberColumns(),
                       values, where);
}

std::vector<RawRow> ManageConversationMember::FindRaw(const RawRow& where) {
  return ExecuteSelect(db::conversation_member::kTable, ConversationMemberColumns(),
                       where);
}

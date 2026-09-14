#include "database/manage_conversation.h"

#include "database/database_schema.h"

namespace {

// Conversation 表允许出现的字段白名单。
const std::vector<std::string>& ConversationColumns() {
  static const std::vector<std::string> kColumns = {
      db::conversation::kConversationId,   db::conversation::kConversationType,
      db::conversation::kConversationName, db::conversation::kOwnerId,
      db::conversation::kPrivateUserLowId, db::conversation::kPrivateUserHighId,
      db::conversation::kAvatarUrl,        db::conversation::kStatus,
      db::conversation::kCreatedAt,        db::conversation::kUpdatedAt,
  };
  return kColumns;
}

// Conversation 表的建表语句。
const char* const kCreateTableSql = R"sql(
CREATE TABLE IF NOT EXISTS `Conversation` (
  `ConversationId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `ConversationType` TINYINT UNSIGNED NOT NULL,
  `ConversationName` VARCHAR(64) DEFAULT NULL,
  `OwnerId` BIGINT UNSIGNED DEFAULT NULL,
  `PrivateUserLowId` BIGINT UNSIGNED DEFAULT NULL,
  `PrivateUserHighId` BIGINT UNSIGNED DEFAULT NULL,
  `AvatarUrl` VARCHAR(512) DEFAULT NULL,
  `Status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `CreatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `UpdatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`ConversationId`),
  UNIQUE KEY `uk_private_conversation`
    (`PrivateUserLowId`, `PrivateUserHighId`),
  KEY `idx_conversation_owner` (`OwnerId`),
  CONSTRAINT `fk_conversation_owner`
    FOREIGN KEY (`OwnerId`) REFERENCES `User` (`UserId`) ON DELETE SET NULL,
  CONSTRAINT `fk_conversation_private_low`
    FOREIGN KEY (`PrivateUserLowId`) REFERENCES `User` (`UserId`),
  CONSTRAINT `fk_conversation_private_high`
    FOREIGN KEY (`PrivateUserHighId`) REFERENCES `User` (`UserId`),
  CONSTRAINT `chk_conversation_type` CHECK (`ConversationType` IN (1, 2)),
  CONSTRAINT `chk_conversation_status` CHECK (`Status` IN (0, 1, 2)),
  CONSTRAINT `chk_private_user_order`
    CHECK (`PrivateUserLowId` IS NULL OR `PrivateUserHighId` IS NULL OR
           `PrivateUserLowId` < `PrivateUserHighId`),
  CONSTRAINT `chk_conversation_membership_shape`
    CHECK ((`ConversationType` = 1 AND `PrivateUserLowId` IS NOT NULL AND
            `PrivateUserHighId` IS NOT NULL) OR
           (`ConversationType` = 2 AND `PrivateUserLowId` IS NULL AND
            `PrivateUserHighId` IS NULL))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
)sql";

}  // namespace

bool ManageConversation::SqlInit() {
  if (!CreateDatabase()) {
    return false;
  }
  return CreateTable(kCreateTableSql);
}

bool ManageConversation::IsRunning() {
  return Ping();
}

bool ManageConversation::AddRaw(const RawRow& row) {
  // 私聊与群聊的具体字段由数据库的 CHECK 约束进一步校验。
  if (!RequireColumns(row, {db::conversation::kConversationType})) {
    return false;
  }
  return ExecuteInsert(db::conversation::kTable, ConversationColumns(), row);
}

bool ManageConversation::DelRaw(const RawRow& where) {
  return ExecuteDelete(db::conversation::kTable, ConversationColumns(), where);
}

bool ManageConversation::UpdateRaw(const RawRow& values, const RawRow& where) {
  return ExecuteUpdate(db::conversation::kTable, ConversationColumns(), values, where);
}

std::vector<RawRow> ManageConversation::FindRaw(const RawRow& where) {
  return ExecuteSelect(db::conversation::kTable, ConversationColumns(), where);
}

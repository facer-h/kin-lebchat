#include "database/manage_message.h"

#include "database/database_schema.h"

namespace {

// Message 表允许出现的字段白名单。
const std::vector<std::string>& MessageColumns() {
  static const std::vector<std::string> kColumns = {
      db::message::kMessageId,      db::message::kConversationId,
      db::message::kSenderId,       db::message::kClientMessageId,
      db::message::kMessageType,    db::message::kContent,
      db::message::kReplyMessageId, db::message::kStatus,
      db::message::kSendTime,
  };
  return kColumns;
}

// Message 表的建表语句。
const char* const kCreateTableSql = R"sql(
CREATE TABLE IF NOT EXISTS `Message` (
  `MessageId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `ConversationId` BIGINT UNSIGNED NOT NULL,
  `SenderId` BIGINT UNSIGNED NOT NULL,
  `ClientMessageId` CHAR(36) NOT NULL,
  `MessageType` TINYINT UNSIGNED NOT NULL,
  `Content` TEXT NOT NULL,
  `ReplyMessageId` BIGINT UNSIGNED DEFAULT NULL,
  `Status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `SendTime` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
  PRIMARY KEY (`MessageId`),
  UNIQUE KEY `uk_sender_client_message` (`SenderId`, `ClientMessageId`),
  KEY `idx_message_conversation` (`ConversationId`, `MessageId`),
  KEY `idx_message_sender` (`SenderId`, `MessageId`),
  KEY `idx_message_reply` (`ReplyMessageId`),
  CONSTRAINT `fk_message_conversation`
    FOREIGN KEY (`ConversationId`) REFERENCES `Conversation` (`ConversationId`),
  CONSTRAINT `fk_message_sender`
    FOREIGN KEY (`SenderId`) REFERENCES `User` (`UserId`),
  CONSTRAINT `fk_message_reply`
    FOREIGN KEY (`ReplyMessageId`) REFERENCES `Message` (`MessageId`)
    ON DELETE SET NULL,
  CONSTRAINT `chk_message_type` CHECK (`MessageType` IN (1, 2, 3, 4, 5, 6)),
  CONSTRAINT `chk_message_status` CHECK (`Status` IN (0, 1, 2))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
)sql";

}  // namespace

bool ManageMessage::SqlInit() {
  if (!CreateDatabase()) {
    return false;
  }
  return CreateTable(kCreateTableSql);
}

bool ManageMessage::IsRunning() {
  return Ping();
}

bool ManageMessage::AddRaw(const RawRow& row) {
  if (!RequireColumns(row, {db::message::kConversationId, db::message::kSenderId,
                            db::message::kClientMessageId,
                            db::message::kMessageType, db::message::kContent})) {
    return false;
  }
  return ExecuteInsert(db::message::kTable, MessageColumns(), row);
}

bool ManageMessage::DelRaw(const RawRow& where) {
  return ExecuteDelete(db::message::kTable, MessageColumns(), where);
}

bool ManageMessage::UpdateRaw(const RawRow& values, const RawRow& where) {
  return ExecuteUpdate(db::message::kTable, MessageColumns(), values, where);
}

std::vector<RawRow> ManageMessage::FindRaw(const RawRow& where) {
  return ExecuteSelect(db::message::kTable, MessageColumns(), where);
}

#include "database/manage_user_friend.h"

#include "database/database_schema.h"

namespace {

// UserFriend 表允许出现的字段白名单。
const std::vector<std::string>& UserFriendColumns() {
  static const std::vector<std::string> kColumns = {
      db::user_friend::kId,        db::user_friend::kUserId,
      db::user_friend::kFriendId,  db::user_friend::kStatus,
      db::user_friend::kRemark,    db::user_friend::kCreatedAt,
      db::user_friend::kUpdatedAt,
  };
  return kColumns;
}

// UserFriend 表的建表语句。
const char* const kCreateTableSql = R"sql(
CREATE TABLE IF NOT EXISTS `UserFriend` (
  `Id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `UserId` BIGINT UNSIGNED NOT NULL,
  `FriendId` BIGINT UNSIGNED NOT NULL,
  `Status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `Remark` VARCHAR(64) DEFAULT NULL,
  `CreatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `UpdatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`Id`),
  UNIQUE KEY `uk_user_friend` (`UserId`, `FriendId`),
  KEY `idx_user_friend_friend_id` (`FriendId`),
  KEY `idx_user_friend_status` (`UserId`, `Status`),
  CONSTRAINT `fk_user_friend_user`
    FOREIGN KEY (`UserId`) REFERENCES `User` (`UserId`) ON DELETE CASCADE,
  CONSTRAINT `fk_user_friend_friend`
    FOREIGN KEY (`FriendId`) REFERENCES `User` (`UserId`) ON DELETE CASCADE,
  CONSTRAINT `chk_user_friend_status`
    CHECK (`Status` IN (0, 1, 2, 3, 4)),
  CONSTRAINT `chk_user_friend_not_self` CHECK (`UserId` <> `FriendId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
)sql";

}  // namespace

bool ManageUserFriend::SqlInit() {
  if (!CreateDatabase()) {
    return false;
  }
  return CreateTable(kCreateTableSql);
}

bool ManageUserFriend::IsRunning() {
  return Ping();
}

bool ManageUserFriend::AddRaw(const RawRow& row) {
  if (!RequireColumns(row, {db::user_friend::kUserId, db::user_friend::kFriendId})) {
    return false;
  }
  return ExecuteInsert(db::user_friend::kTable, UserFriendColumns(), row);
}

bool ManageUserFriend::DelRaw(const RawRow& where) {
  return ExecuteDelete(db::user_friend::kTable, UserFriendColumns(), where);
}

bool ManageUserFriend::UpdateRaw(const RawRow& values, const RawRow& where) {
  return ExecuteUpdate(db::user_friend::kTable, UserFriendColumns(), values, where);
}

std::vector<RawRow> ManageUserFriend::FindRaw(const RawRow& where) {
  return ExecuteSelect(db::user_friend::kTable, UserFriendColumns(), where);
}

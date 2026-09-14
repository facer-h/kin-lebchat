#include "database/manage_user.h"

#include "database/database_schema.h"

namespace {

// User 表允许出现的字段白名单。
const std::vector<std::string>& UserColumns() {
  static const std::vector<std::string> kColumns = {
      db::user::kUserId,    db::user::kUserAccount, db::user::kUserPassword,
      db::user::kNickname,  db::user::kAvatarUrl,   db::user::kStatus,
      db::user::kCreatedAt, db::user::kUpdatedAt,
  };
  return kColumns;
}

// User 表的建表语句。
const char* const kCreateTableSql = R"sql(
CREATE TABLE IF NOT EXISTS `User` (
  `UserId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `UserAccount` VARCHAR(64) NOT NULL,
  `UserPassword` VARCHAR(255) NOT NULL,
  `Nickname` VARCHAR(64) DEFAULT NULL,
  `AvatarUrl` VARCHAR(512) DEFAULT NULL,
  `Status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `CreatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `UpdatedAt` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`UserId`),
  UNIQUE KEY `uk_user_account` (`UserAccount`),
  CONSTRAINT `chk_user_status` CHECK (`Status` IN (0, 1, 2))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
)sql";

}  // namespace

bool ManageUser::SqlInit() {
  if (!CreateDatabase()) {
    return false;
  }
  return CreateTable(kCreateTableSql);
}

bool ManageUser::IsRunning() {
  return Ping();
}

bool ManageUser::AddRaw(const RawRow& row) {
  if (!RequireColumns(row, {db::user::kUserAccount, db::user::kUserPassword})) {
    return false;
  }
  return ExecuteInsert(db::user::kTable, UserColumns(), row);
}

bool ManageUser::DelRaw(const RawRow& where) {
  return ExecuteDelete(db::user::kTable, UserColumns(), where);
}

bool ManageUser::UpdateRaw(const RawRow& values, const RawRow& where) {
  return ExecuteUpdate(db::user::kTable, UserColumns(), values, where);
}

std::vector<RawRow> ManageUser::FindRaw(const RawRow& where) {
  return ExecuteSelect(db::user::kTable, UserColumns(), where);
}

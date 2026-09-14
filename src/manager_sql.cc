#include "database/manager_sql.h"

#include <mysql/mysql.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

namespace {

// 用反引号包裹表名或字段名，避免与 MySQL 关键字冲突。
std::string Quote(const std::string& identifier) {
  return "`" + identifier + "`";
}

}  // namespace

ManagerSql::ManagerSql(DatabaseConfig config) : config_(std::move(config)) {}

ManagerSql::~ManagerSql() {
  std::lock_guard<std::mutex> lock(mutex_);
  CloseLocked();
}

std::string ManagerSql::LastError() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_error_;
}

bool ManagerSql::CreateDatabase() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!ConnectLocked(false)) {
    return false;
  }
  const std::string sql = "CREATE DATABASE IF NOT EXISTS " +
                          Quote(config_.database) +
                          " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci";
  return ExecuteSqlLocked(sql);
}

bool ManagerSql::CreateTable(const std::string& create_table_sql) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!ConnectLocked(true)) {
    return false;
  }
  return ExecuteSqlLocked(create_table_sql);
}

bool ManagerSql::Ping() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (connection_ == nullptr) {
    return ConnectLocked(true);
  }
  if (mysql_ping(connection_) != 0) {
    CloseLocked();
    return ConnectLocked(true);
  }
  return true;
}

bool ManagerSql::RequireColumns(const RawRow& row,
                                const std::vector<std::string>& required_columns) {
  for (const std::string& column : required_columns) {
    if (row.find(column) == row.end()) {
      std::lock_guard<std::mutex> lock(mutex_);
      SetLastErrorLocked("缺少必填字段：" + column);
      return false;
    }
  }
  return true;
}

bool ManagerSql::ExecuteInsert(const std::string& table,
                               const std::vector<std::string>& allowed_columns,
                               const RawRow& row) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (connection_ == nullptr && !ConnectLocked(true)) {
    return false;
  }
  if (row.empty()) {
    SetLastErrorLocked("插入的行不能为空");
    return false;
  }
  if (!ValidateRow(allowed_columns, row)) {
    SetLastErrorLocked("插入的行包含表 " + table + " 不允许的字段");
    return false;
  }

  std::string sql = "INSERT INTO " + Quote(table) + " (";
  std::vector<std::string> parameters;
  bool first = true;
  for (const auto& entry : row) {
    if (!first) {
      sql += ", ";
    }
    sql += Quote(entry.first);
    parameters.push_back(entry.second);
    first = false;
  }
  sql += ") VALUES (";
  for (std::size_t i = 0; i < row.size(); ++i) {
    if (i > 0) {
      sql += ", ";
    }
    sql += "?";
  }
  sql += ")";

  return ExecutePreparedLocked(sql, parameters);
}

bool ManagerSql::ExecuteDelete(const std::string& table,
                               const std::vector<std::string>& allowed_columns,
                               const RawRow& where) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (connection_ == nullptr && !ConnectLocked(true)) {
    return false;
  }
  if (where.empty()) {
    SetLastErrorLocked("删除操作必须提供非空的 where 条件");
    return false;
  }
  if (!ValidateRow(allowed_columns, where)) {
    SetLastErrorLocked("删除的 where 条件包含表 " + table + " 不允许的字段");
    return false;
  }

  std::string sql = "DELETE FROM " + Quote(table) + " WHERE ";
  std::vector<std::string> parameters;
  bool first = true;
  for (const auto& entry : where) {
    if (!first) {
      sql += " AND ";
    }
    sql += Quote(entry.first) + " = ?";
    parameters.push_back(entry.second);
    first = false;
  }

  return ExecutePreparedLocked(sql, parameters);
}

bool ManagerSql::ExecuteUpdate(const std::string& table,
                               const std::vector<std::string>& allowed_columns,
                               const RawRow& values, const RawRow& where) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (connection_ == nullptr && !ConnectLocked(true)) {
    return false;
  }
  if (values.empty()) {
    SetLastErrorLocked("更新操作的值不能为空");
    return false;
  }
  if (where.empty()) {
    SetLastErrorLocked("更新操作必须提供非空的 where 条件");
    return false;
  }
  if (!ValidateRow(allowed_columns, values) ||
      !ValidateRow(allowed_columns, where)) {
    SetLastErrorLocked("更新操作包含表 " + table + " 不允许的字段");
    return false;
  }

  std::string sql = "UPDATE " + Quote(table) + " SET ";
  std::vector<std::string> parameters;
  bool first = true;
  for (const auto& entry : values) {
    if (!first) {
      sql += ", ";
    }
    sql += Quote(entry.first) + " = ?";
    parameters.push_back(entry.second);
    first = false;
  }
  sql += " WHERE ";
  first = true;
  for (const auto& entry : where) {
    if (!first) {
      sql += " AND ";
    }
    sql += Quote(entry.first) + " = ?";
    parameters.push_back(entry.second);
    first = false;
  }

  return ExecutePreparedLocked(sql, parameters);
}

std::vector<RawRow> ManagerSql::ExecuteSelect(
    const std::string& table, const std::vector<std::string>& allowed_columns,
    const RawRow& where) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<RawRow> rows;
  if (connection_ == nullptr && !ConnectLocked(true)) {
    return rows;
  }
  if (!where.empty() && !ValidateRow(allowed_columns, where)) {
    SetLastErrorLocked("查询的 where 条件包含表 " + table + " 不允许的字段");
    return rows;
  }

  std::string sql = "SELECT * FROM " + Quote(table);
  std::vector<std::string> parameters;
  if (!where.empty()) {
    sql += " WHERE ";
    bool first = true;
    for (const auto& entry : where) {
      if (!first) {
        sql += " AND ";
      }
      sql += Quote(entry.first) + " = ?";
      parameters.push_back(entry.second);
      first = false;
    }
  }

  MYSQL_STMT* stmt = mysql_stmt_init(connection_);
  if (stmt == nullptr) {
    SetLastErrorLocked("mysql_stmt_init 失败");
    return rows;
  }
  if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0) {
    SetLastErrorLocked(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return rows;
  }

  std::vector<MYSQL_BIND> binds(parameters.size());
  std::vector<unsigned long> paramLengths(parameters.size());
  for (std::size_t i = 0; i < parameters.size(); ++i) {
    std::memset(&binds[i], 0, sizeof(MYSQL_BIND));
    paramLengths[i] = parameters[i].size();
    binds[i].buffer_type = MYSQL_TYPE_STRING;
    binds[i].buffer = const_cast<char*>(parameters[i].data());
    binds[i].buffer_length = paramLengths[i];
    binds[i].length = &paramLengths[i];
  }
  if (!parameters.empty() && mysql_stmt_bind_param(stmt, binds.data()) != 0) {
    SetLastErrorLocked(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return rows;
  }
  if (mysql_stmt_execute(stmt) != 0) {
    SetLastErrorLocked(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return rows;
  }

  MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
  if (meta == nullptr) {
    SetLastErrorLocked(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return rows;
  }
  const unsigned int fieldCount = mysql_num_fields(meta);
  MYSQL_FIELD* fields = mysql_fetch_fields(meta);

  // 第一版为每列使用固定 64 KiB 缓冲；超长字段会被截断，后续可优化。
  constexpr std::size_t kBufferSize = 64 * 1024;
  std::vector<std::array<char, kBufferSize>> buffers(fieldCount);
  std::vector<unsigned long> resultLengths(fieldCount);
  std::vector<my_bool> isNull(fieldCount);
  std::vector<MYSQL_BIND> resultBinds(fieldCount);
  for (unsigned int i = 0; i < fieldCount; ++i) {
    std::memset(&resultBinds[i], 0, sizeof(MYSQL_BIND));
    resultBinds[i].buffer_type = MYSQL_TYPE_STRING;
    resultBinds[i].buffer = buffers[i].data();
    resultBinds[i].buffer_length = kBufferSize;
    resultBinds[i].length = &resultLengths[i];
    resultBinds[i].is_null = &isNull[i];
  }
  if (fieldCount > 0 &&
      mysql_stmt_bind_result(stmt, resultBinds.data()) != 0) {
    SetLastErrorLocked(mysql_stmt_error(stmt));
    mysql_free_result(meta);
    mysql_stmt_close(stmt);
    return rows;
  }

  while (true) {
    const int rc = mysql_stmt_fetch(stmt);
    if (rc == MYSQL_NO_DATA) {
      break;
    }
    if (rc != 0 && rc != MYSQL_DATA_TRUNCATED) {
      SetLastErrorLocked(mysql_stmt_error(stmt));
      break;
    }
    RawRow row;
    for (unsigned int i = 0; i < fieldCount; ++i) {
      if (isNull[i]) {
        row[fields[i].name] = "";
      } else {
        const std::size_t len =
            std::min<std::size_t>(resultLengths[i], kBufferSize);
        row[fields[i].name] = std::string(buffers[i].data(), len);
      }
    }
    rows.push_back(std::move(row));
  }

  mysql_free_result(meta);
  mysql_stmt_close(stmt);
  return rows;
}

bool ManagerSql::ConnectLocked(bool select_database) {
  if (connection_ != nullptr) {
    return true;
  }
  MYSQL* conn = mysql_init(nullptr);
  if (conn == nullptr) {
    SetLastErrorLocked("mysql_init 失败");
    return false;
  }
  const char* database = select_database ? config_.database.c_str() : nullptr;
  if (mysql_real_connect(conn, config_.host.c_str(), config_.user.c_str(),
                         config_.password.c_str(), database, config_.port,
                         nullptr, 0) == nullptr) {
    SetLastErrorLocked(mysql_error(conn));
    mysql_close(conn);
    return false;
  }
  connection_ = conn;
  return true;
}

void ManagerSql::CloseLocked() {
  if (connection_ != nullptr) {
    mysql_close(connection_);
    connection_ = nullptr;
  }
}

bool ManagerSql::ExecuteSqlLocked(const std::string& sql) {
  if (mysql_real_query(connection_, sql.c_str(), sql.size()) != 0) {
    SetLastErrorLocked(mysql_error(connection_));
    return false;
  }
  return true;
}

bool ManagerSql::ExecutePreparedLocked(
    const std::string& sql, const std::vector<std::string>& parameters) {
  MYSQL_STMT* stmt = mysql_stmt_init(connection_);
  if (stmt == nullptr) {
    SetLastErrorLocked("mysql_stmt_init 失败");
    return false;
  }
  if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0) {
    SetLastErrorLocked(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }

  std::vector<MYSQL_BIND> binds(parameters.size());
  std::vector<unsigned long> lengths(parameters.size());
  for (std::size_t i = 0; i < parameters.size(); ++i) {
    std::memset(&binds[i], 0, sizeof(MYSQL_BIND));
    lengths[i] = parameters[i].size();
    binds[i].buffer_type = MYSQL_TYPE_STRING;
    binds[i].buffer = const_cast<char*>(parameters[i].data());
    binds[i].buffer_length = lengths[i];
    binds[i].length = &lengths[i];
  }

  bool ok = true;
  if (!parameters.empty() && mysql_stmt_bind_param(stmt, binds.data()) != 0) {
    SetLastErrorLocked(mysql_stmt_error(stmt));
    ok = false;
  }
  if (ok && mysql_stmt_execute(stmt) != 0) {
    SetLastErrorLocked(mysql_stmt_error(stmt));
    ok = false;
  }

  mysql_stmt_close(stmt);
  return ok;
}

bool ManagerSql::ValidateRow(const std::vector<std::string>& allowed_columns,
                             const RawRow& row) {
  for (const auto& entry : row) {
    bool found = false;
    for (const std::string& column : allowed_columns) {
      if (entry.first == column) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

void ManagerSql::SetLastErrorLocked(const std::string& error) {
  last_error_ = error;
}

#ifndef KINCHAT_HEADER_DATABASE_DATABASE_CONFIG_H_
#define KINCHAT_HEADER_DATABASE_DATABASE_CONFIG_H_

#include <string>

#include "database/database_schema.h"

// MySQL 连接配置。
struct DatabaseConfig {
  std::string host = "127.0.0.1";
  std::string user = "root";
  std::string password;
  unsigned int port = 3306;
  std::string database = db::kDatabaseName;
};

#endif  // KINCHAT_HEADER_DATABASE_DATABASE_CONFIG_H_

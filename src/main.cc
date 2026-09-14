#include <iostream>
#include <string>
#include <vector>

#include "database/database_config.h"
#include "database/database_schema.h"
#include "database/manage_user.h"
#include "database/raw_row.h"

int main(int argc, char* argv[]) {
  // 通过命令行参数指定数据库地址、用户和密码，方便在虚拟机上连接 Windows 的 MySQL。
  DatabaseConfig config;
  if (argc > 1) {
    config.host = argv[1];
  }
  if (argc > 2) {
    config.user = argv[2];
  }
  if (argc > 3) {
    config.password = argv[3];
  }

  ManageUser users(config);
  if (!users.SqlInit()) {
    std::cerr << "初始化失败：" << users.LastError() << '\n';
    return 1;
  }
  std::cout << "数据库和 User 表已就绪\n";

  RawRow row;
  row[db::user::kUserAccount] = "alice";
  row[db::user::kUserPassword] = "hashed_password";
  row[db::user::kNickname] = "Alice";
  if (!users.AddRaw(row)) {
    std::cerr << "插入用户失败：" << users.LastError() << '\n';
    return 1;
  }
  std::cout << "已插入测试用户\n";

  const std::vector<RawRow> result = users.FindRaw({});
  std::cout << "User 表共有 " << result.size() << " 条记录\n";
  for (const RawRow& oneRow : result) {
    const auto it = oneRow.find(db::user::kUserAccount);
    if (it != oneRow.end()) {
      std::cout << "  账号：" << it->second << '\n';
    }
  }
  return 0;
}

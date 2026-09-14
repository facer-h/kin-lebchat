#ifndef KINCHAT_HEADER_DATABASE_MANAGE_USER_H_
#define KINCHAT_HEADER_DATABASE_MANAGE_USER_H_

#include "database/manager_sql.h"

// 管理 User 表。
class ManageUser final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 User 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 User 表使用的数据库连接是否正常。 */
  bool IsRunning() override;
  /*
   * 参数：row，必须包含 UserAccount、UserPassword，可包含 Nickname、
   *       AvatarUrl、Status；UserPassword 必须保存密码哈希值。
   * 功能：增加一名用户。
   */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，用户编号或账号。功能：删除符合条件的用户。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，待修改字段；where，用户条件。功能：修改用户信息。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，用户查询条件。功能：查询用户记录。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

#endif  // KINCHAT_HEADER_DATABASE_MANAGE_USER_H_

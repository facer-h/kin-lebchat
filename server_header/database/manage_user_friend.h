#ifndef KINCHAT_HEADER_DATABASE_MANAGE_USER_FRIEND_H_
#define KINCHAT_HEADER_DATABASE_MANAGE_USER_FRIEND_H_

#include "database/manager_sql.h"

// 管理 UserFriend 表。
class ManageUserFriend final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 UserFriend 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 UserFriend 表使用的数据库连接是否正常。 */
  bool IsRunning() override;
  /*
   * 参数：row，必须包含 UserId、FriendId，可包含 Status、Remark。
   * 功能：增加好友申请或联系人关系，两个用户编号均关联 User.UserId。
   */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，好友关系条件。功能：删除符合条件的好友关系。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，关系修改字段；where，关系条件。功能：修改好友关系。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，好友查询条件。功能：查询好友关系。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

#endif  // KINCHAT_HEADER_DATABASE_MANAGE_USER_FRIEND_H_

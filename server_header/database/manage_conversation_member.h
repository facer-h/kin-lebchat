#ifndef KINCHAT_HEADER_DATABASE_MANAGE_CONVERSATION_MEMBER_H_
#define KINCHAT_HEADER_DATABASE_MANAGE_CONVERSATION_MEMBER_H_

#include "database/manager_sql.h"

// 管理 ConversationMember 表。
class ManageConversationMember final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 ConversationMember 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 ConversationMember 表的数据库连接是否正常。 */
  bool IsRunning() override;
  /*
   * 参数：row，必须包含 ConversationId、UserId，可包含 Role、MuteStatus、
   *       LastReadMessageId。
   * 功能：建立用户和会话的成员关系，两个必填字段组成联合主键。
   */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，会话和成员条件。功能：从会话中删除符合条件的成员。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，成员修改字段；where，成员条件。功能：修改成员记录。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，会话或成员查询条件。功能：查询会话成员记录。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

#endif  // KINCHAT_HEADER_DATABASE_MANAGE_CONVERSATION_MEMBER_H_

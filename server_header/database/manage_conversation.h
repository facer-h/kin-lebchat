#ifndef KINCHAT_HEADER_DATABASE_MANAGE_CONVERSATION_H_
#define KINCHAT_HEADER_DATABASE_MANAGE_CONVERSATION_H_

#include "database/manager_sql.h"

// 管理 Conversation 表。
class ManageConversation final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 Conversation 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 Conversation 表使用的数据库连接是否正常。 */
  bool IsRunning() override;
  /*
   * 参数：row，必须包含 ConversationType；私聊还必须包含排序后的
   *       PrivateUserLowId、PrivateUserHighId；群聊必须包含
   *       ConversationName、OwnerId；两种会话均可包含 AvatarUrl、Status。
   * 功能：创建私聊或群聊会话。
   */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，会话编号等条件。功能：删除符合条件的会话。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，会话修改字段；where，会话条件。功能：修改会话信息。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，会话查询条件。功能：查询会话记录。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

#endif  // KINCHAT_HEADER_DATABASE_MANAGE_CONVERSATION_H_

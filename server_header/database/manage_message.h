#ifndef KINCHAT_HEADER_DATABASE_MANAGE_MESSAGE_H_
#define KINCHAT_HEADER_DATABASE_MANAGE_MESSAGE_H_

#include "database/manager_sql.h"

// 管理 Message 表。
class ManageMessage final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 Message 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 Message 表使用的数据库连接是否正常。 */
  bool IsRunning() override;
  /*
   * 参数：row，必须包含 ConversationId、SenderId、ClientMessageId、
   *       MessageType、Content，可包含 ReplyMessageId、Status。
   * 功能：保存私聊或群聊消息。
   */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，消息编号等条件。功能：删除符合条件的消息。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，消息修改字段；where，消息条件。功能：修改消息记录。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，会话、发送者或消息编号。功能：查询消息记录。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

#endif  // KINCHAT_HEADER_DATABASE_MANAGE_MESSAGE_H_

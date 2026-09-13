#ifndef MANAGER_SQL_H_
#define MANAGER_SQL_H_

#include <mysql/mysql.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

// 一行数据库记录，键为字段名，值为字段内容。
using RawRow = std::map<std::string, std::string>;

// MySQL 连接配置。
struct DatabaseConfig {
  std::string host = "127.0.0.1";
  std::string user = "root";
  std::string password;
  unsigned int port = 3306;
  std::string database = "User";
};

// 所有数据表管理类的抽象父类。
class ManagerSql {
 public:
  /*
   * 参数：config，MySQL 的地址、用户、密码、端口和数据库名称。
   * 功能：保存数据库连接配置，为子类访问 MySQL 做准备。
   */
  explicit ManagerSql(DatabaseConfig config);

  /*
   * 参数：无。
   * 功能：通过基类指针销毁子类对象时，正确释放数据库连接等资源。
   */
  virtual ~ManagerSql();

  ManagerSql(const ManagerSql&) = delete;
  ManagerSql& operator=(const ManagerSql&) = delete;

  /*
   * 参数：无。
   * 功能：创建数据库以及当前子类负责的数据表，由每个子类实现。
   */
  virtual bool SqlInit() = 0;

  /*
   * 参数：无。
   * 功能：检查 MySQL 服务和当前数据库连接是否正常，由每个子类实现。
   */
  virtual bool IsRunning() = 0;

  /*
   * 参数：row，需要增加的字段和值。
   * 功能：向当前子类负责的数据表增加一行记录，由每个子类实现。
   */
  virtual bool AddRaw(const RawRow& row) = 0;

  /*
   * 参数：where，删除条件中的字段和值，不能为空。
   * 功能：删除当前子类负责的数据表中符合条件的记录，由每个子类实现。
   */
  virtual bool DelRaw(const RawRow& where) = 0;

  /*
   * 参数：values，需要修改的字段和值；where，筛选条件，不能为空。
   * 功能：修改当前子类负责的数据表中符合条件的记录，由每个子类实现。
   */
  virtual bool UpdateRaw(const RawRow& values, const RawRow& where) = 0;

  /*
   * 参数：where，查询条件；传入空集合时查询当前表中的全部记录。
   * 功能：查询当前子类负责的数据表，并返回满足条件的记录。
   */
  virtual std::vector<RawRow> FindRaw(const RawRow& where = {}) = 0;

  /*
   * 参数：无。
   * 功能：返回最近一次数据库连接、字段校验或 SQL 执行错误。
   */
  std::string LastError() const;

 protected:
  /*
   * 参数：无。
   * 功能：根据 DatabaseConfig 创建数据库，供子类的 SqlInit 调用。
   */
  bool CreateDatabase();

  /*
   * 参数：create_table_sql，当前子类负责的数据表建表语句。
   * 功能：连接指定数据库并执行建表语句。
   */
  bool CreateTable(const std::string& create_table_sql);

  /*
   * 参数：无。
   * 功能：检查连接是否可用；连接断开时尝试重新建立连接。
   */
  bool Ping();

  /*
   * 参数：row，待检查的数据；required_columns，当前操作要求的字段集合。
   * 功能：检查一行数据是否包含当前表操作所需的全部字段。
   */
  bool RequireColumns(const RawRow& row,
                      const std::vector<std::string>& required_columns);

  /*
   * 参数：table，目标表名；allowed_columns，合法字段白名单；row，新增数据。
   * 功能：生成参数化 INSERT 语句并执行，供各个表管理子类复用。
   */
  bool ExecuteInsert(const std::string& table,
                     const std::vector<std::string>& allowed_columns,
                     const RawRow& row);

  /*
   * 参数：table，目标表名；allowed_columns，合法字段白名单；where，删除条件。
   * 功能：生成参数化 DELETE 语句并执行；where 为空时必须拒绝执行。
   */
  bool ExecuteDelete(const std::string& table,
                     const std::vector<std::string>& allowed_columns,
                     const RawRow& where);

  /*
   * 参数：table，目标表名；allowed_columns，合法字段白名单；
   *       values，修改后的字段和值；where，筛选条件。
   * 功能：生成参数化 UPDATE 语句并执行；where 为空时必须拒绝执行。
   */
  bool ExecuteUpdate(const std::string& table,
                     const std::vector<std::string>& allowed_columns,
                     const RawRow& values, const RawRow& where);

  /*
   * 参数：table，目标表名；allowed_columns，合法字段白名单；where，查询条件。
   * 功能：生成参数化 SELECT 语句并执行，将结果转换为 RawRow 集合。
   */
  std::vector<RawRow> ExecuteSelect(
      const std::string& table, const std::vector<std::string>& allowed_columns,
      const RawRow& where);

 private:
  /*
   * 参数：select_database，是否在连接成功后选择配置中的数据库。
   * 功能：建立或恢复 MySQL 连接；调用者必须已经持有 mutex_。
   */
  bool ConnectLocked(bool select_database);

  /*
   * 参数：无。
   * 功能：关闭 MySQL 连接；调用者必须已经持有 mutex_。
   */
  void CloseLocked();

  /*
   * 参数：sql，不含外部输入的完整 SQL 语句。
   * 功能：执行不需要绑定参数的 SQL；调用者必须已经持有 mutex_。
   */
  bool ExecuteSqlLocked(const std::string& sql);

  /*
   * 参数：sql，带占位符的 SQL；parameters，按占位符顺序排列的参数。
   * 功能：使用 MySQL 预处理语句绑定参数并执行 SQL。
   */
  bool ExecutePreparedLocked(const std::string& sql,
                             const std::vector<std::string>& parameters);

  /*
   * 参数：allowed_columns，字段白名单；row，需要检查的一行数据。
   * 功能：拒绝当前数据表不允许使用的字段，避免非法字段进入 SQL。
   */
  bool ValidateRow(const std::vector<std::string>& allowed_columns,
                   const RawRow& row);

  /*
   * 参数：error，需要保存的错误信息。
   * 功能：保存最近一次错误；调用者必须已经持有 mutex_。
   */
  void SetLastErrorLocked(const std::string& error);

  MYSQL* connection_ = nullptr;
  DatabaseConfig config_;
  mutable std::mutex mutex_;
  std::string last_error_;
};

// 管理 User 表。
class ManageUser final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 User 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 User 表使用的数据库连接是否正常。 */
  bool IsRunning() override;
  /* 参数：row，账号和密码哈希等字段。功能：增加一名用户。 */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，用户编号或账号。功能：删除符合条件的用户。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，待修改字段；where，用户条件。功能：修改用户信息。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，用户查询条件。功能：查询用户记录。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

// 管理 UserFriend 表。
class ManageUserFriend final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 UserFriend 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 UserFriend 表使用的数据库连接是否正常。 */
  bool IsRunning() override;
  /* 参数：row，用户编号、好友编号和状态。功能：增加好友关系。 */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，好友关系条件。功能：删除符合条件的好友关系。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，关系状态等字段；where，关系条件。功能：修改好友关系。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，好友查询条件。功能：查询好友关系。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

// 管理 Conversation 表。
class ManageConversation final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 Conversation 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 Conversation 表使用的数据库连接是否正常。 */
  bool IsRunning() override;
  /* 参数：row，会话类型等字段。功能：创建一条会话记录。 */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，会话编号等条件。功能：删除符合条件的会话。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，会话修改字段；where，会话条件。功能：修改会话信息。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，会话查询条件。功能：查询会话记录。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

// 管理 ConversationMember 表。
class ManageConversationMember final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 ConversationMember 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 ConversationMember 表的数据库连接是否正常。 */
  bool IsRunning() override;
  /* 参数：row，会话编号和用户编号。功能：向会话增加一名成员。 */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，会话和成员条件。功能：从会话中删除符合条件的成员。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，成员修改字段；where，成员条件。功能：修改会话成员记录。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，会话或成员查询条件。功能：查询会话成员记录。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

// 管理 Message 表。
class ManageMessage final : public ManagerSql {
 public:
  using ManagerSql::ManagerSql;

  /* 参数：无。功能：创建 User 数据库和 Message 表。 */
  bool SqlInit() override;
  /* 参数：无。功能：检查 Message 表使用的数据库连接是否正常。 */
  bool IsRunning() override;
  /* 参数：row，会话、发送者、消息编号、类型和内容。功能：增加一条消息。 */
  bool AddRaw(const RawRow& row) override;
  /* 参数：where，消息编号等条件。功能：删除符合条件的消息。 */
  bool DelRaw(const RawRow& where) override;
  /* 参数：values，消息修改字段；where，消息条件。功能：修改消息记录。 */
  bool UpdateRaw(const RawRow& values, const RawRow& where) override;
  /* 参数：where，会话、发送者或消息编号。功能：查询消息记录。 */
  std::vector<RawRow> FindRaw(const RawRow& where = {}) override;
};

// 兼容之前代码中已经使用的拼写。
using MangerSql = ManagerSql;
using MangeUser = ManageUser;

#endif  // MANAGER_SQL_H_

#ifndef KINCHAT_HEADER_DATABASE_MANAGER_SQL_H_
#define KINCHAT_HEADER_DATABASE_MANAGER_SQL_H_

#include <mutex>
#include <string>
#include <vector>

#include "database/database_config.h"
#include "database/raw_row.h"

// MySQL C API 中连接对象的底层结构；完整定义只在 .cc 中引入。
struct st_mysql;

// 所有数据表管理类的抽象父类，只负责连接和通用 SQL 执行能力。
class ManagerSql {
 public:
  /*
   * 参数：config，MySQL 的地址、用户、密码、端口和数据库名称。
   * 功能：保存数据库连接配置，为子类访问 MySQL 做准备。
   */
  explicit ManagerSql(DatabaseConfig config);

  /* 参数：无。功能：正确释放数据库连接等资源。 */
  virtual ~ManagerSql();

  ManagerSql(const ManagerSql&) = delete;
  ManagerSql& operator=(const ManagerSql&) = delete;

  /* 参数：无。功能：创建数据库和当前子类负责的数据表。 */
  virtual bool SqlInit() = 0;
  /* 参数：无。功能：检查数据库连接是否正常。 */
  virtual bool IsRunning() = 0;
  /* 参数：row，待增加的字段和值。功能：向当前表增加一行记录。 */
  virtual bool AddRaw(const RawRow& row) = 0;
  /* 参数：where，非空删除条件。功能：删除当前表中符合条件的记录。 */
  virtual bool DelRaw(const RawRow& where) = 0;
  /*
   * 参数：values，待修改字段和值；where，非空筛选条件。
   * 功能：修改当前表中符合条件的记录。
   */
  virtual bool UpdateRaw(const RawRow& values, const RawRow& where) = 0;
  /*
   * 参数：where，查询条件，空集合表示查询全部。
   * 功能：查询当前表并返回满足条件的记录。
   */
  virtual std::vector<RawRow> FindRaw(const RawRow& where = {}) = 0;

  /* 参数：无。功能：返回最近一次数据库操作错误。 */
  std::string LastError() const;

 protected:
  /* 参数：无。功能：根据 DatabaseConfig 创建数据库。 */
  bool CreateDatabase();
  /* 参数：create_table_sql，建表语句。功能：选择数据库并创建数据表。 */
  bool CreateTable(const std::string& create_table_sql);
  /* 参数：无。功能：检查连接，断开时尝试重新连接。 */
  bool Ping();
  /*
   * 参数：row，待检查记录；required_columns，必须出现的字段。
   * 功能：检查记录是否包含当前操作所需的全部字段。
   */
  bool RequireColumns(const RawRow& row,
                      const std::vector<std::string>& required_columns);
  /*
   * 参数：table，表名；allowed_columns，字段白名单；row，新增记录。
   * 功能：生成参数化 INSERT 语句并执行。
   */
  bool ExecuteInsert(const std::string& table,
                     const std::vector<std::string>& allowed_columns,
                     const RawRow& row);
  /*
   * 参数：table，表名；allowed_columns，字段白名单；where，删除条件。
   * 功能：生成参数化 DELETE 语句并执行，拒绝空条件。
   */
  bool ExecuteDelete(const std::string& table,
                     const std::vector<std::string>& allowed_columns,
                     const RawRow& where);
  /*
   * 参数：table，表名；allowed_columns，字段白名单；values，修改值；
   *       where，筛选条件。
   * 功能：生成参数化 UPDATE 语句并执行，拒绝空条件。
   */
  bool ExecuteUpdate(const std::string& table,
                     const std::vector<std::string>& allowed_columns,
                     const RawRow& values, const RawRow& where);
  /*
   * 参数：table，表名；allowed_columns，字段白名单；where，查询条件。
   * 功能：生成参数化 SELECT 语句并返回查询结果。
   */
  std::vector<RawRow> ExecuteSelect(
      const std::string& table, const std::vector<std::string>& allowed_columns,
      const RawRow& where);

 private:
  /*
   * 参数：select_database，连接后是否选择配置中的数据库。
   * 功能：建立或恢复连接；调用者必须持有 mutex_。
   */
  bool ConnectLocked(bool select_database);
  /* 参数：无。功能：关闭连接；调用者必须持有 mutex_。 */
  void CloseLocked();
  /*
   * 参数：sql，不含外部输入的完整 SQL。
   * 功能：执行普通 SQL；调用者必须持有 mutex_。
   */
  bool ExecuteSqlLocked(const std::string& sql);
  /*
   * 参数：sql，带占位符的 SQL；parameters，按顺序排列的参数。
   * 功能：使用 MySQL 预处理语句绑定参数并执行。
   */
  bool ExecutePreparedLocked(const std::string& sql,
                             const std::vector<std::string>& parameters);
  /*
   * 参数：allowed_columns，字段白名单；row，待检查记录。
   * 功能：拒绝当前数据表不允许使用的字段。
   */
  bool ValidateRow(const std::vector<std::string>& allowed_columns,
                   const RawRow& row);
  /*
   * 参数：error，错误信息。
   * 功能：保存最近一次错误；调用者必须持有 mutex_。
   */
  void SetLastErrorLocked(const std::string& error);

  st_mysql* connection_ = nullptr;
  DatabaseConfig config_;
  mutable std::mutex mutex_;
  std::string last_error_;
};

#endif  // KINCHAT_HEADER_DATABASE_MANAGER_SQL_H_

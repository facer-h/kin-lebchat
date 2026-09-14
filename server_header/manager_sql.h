#ifndef KINCHAT_HEADER_MANAGER_SQL_COMPAT_H_
#define KINCHAT_HEADER_MANAGER_SQL_COMPAT_H_

// 数据库模块兼容入口；新代码应按需包含 database/ 下的具体头文件。
#include "database/manage_conversation.h"
#include "database/manage_conversation_member.h"
#include "database/manage_message.h"
#include "database/manage_user.h"
#include "database/manage_user_friend.h"

// 兼容此前代码中已经使用的拼写，后续新代码应使用正确类名。
using MangerSql = ManagerSql;
using MangeUser = ManageUser;

#endif  // KINCHAT_HEADER_MANAGER_SQL_COMPAT_H_

# KinChat 数据库设计

## 1. 文档目的

本文档描述 KinChat 服务端第一版数据库结构，覆盖以下核心业务：

- 用户注册与登录。
- 联系人申请、同意、拒绝、删除和拉黑。
- 私聊会话。
- 群聊创建、成员和权限管理。
- 消息保存、去重、回复、撤回和未读位置。

数据库结构的唯一真实来源是：

```text
database/migrations/001_initial_schema.sql
```

C++ 表名、字段名和状态枚举定义在：

```text
server_header/database/database_schema.h
```

`server_header/database/manager_sql.h` 只定义数据库访问父类和通用执行接口；每张表的管理类分别放在 `server_header/database/manage_*.h` 中，不重复维护字段定义。完整头文件划分参见 `docs/header-structure.md`。

## 2. 数据库总览

数据库名称为 `User`，第一版包含五张表。

| 表名 | 管理类 | 主要职责 |
|---|---|---|
| `User` | `ManageUser` | 用户账号、密码哈希和基础资料 |
| `UserFriend` | `ManageUserFriend` | 好友申请、联系人、备注和拉黑关系 |
| `Conversation` | `ManageConversation` | 私聊和群聊的基本信息 |
| `ConversationMember` | `ManageConversationMember` | 会话成员、角色、禁言和已读位置 |
| `Message` | `ManageMessage` | 私聊与群聊消息 |

## 3. 表关系

| 来源字段 | 目标字段 | 关系 |
|---|---|---|
| `UserFriend.UserId` | `User.UserId` | 当前用户 |
| `UserFriend.FriendId` | `User.UserId` | 联系人或申请目标 |
| `Conversation.OwnerId` | `User.UserId` | 群主 |
| `Conversation.PrivateUserLowId` | `User.UserId` | 私聊中编号较小的用户 |
| `Conversation.PrivateUserHighId` | `User.UserId` | 私聊中编号较大的用户 |
| `ConversationMember.ConversationId` | `Conversation.ConversationId` | 成员所属会话 |
| `ConversationMember.UserId` | `User.UserId` | 会话成员 |
| `Message.ConversationId` | `Conversation.ConversationId` | 消息所属会话 |
| `Message.SenderId` | `User.UserId` | 消息发送者 |
| `Message.ReplyMessageId` | `Message.MessageId` | 被回复的消息 |

核心关系为：

```text
User ──< UserFriend
User ──< ConversationMember >── Conversation ──< Message
User ──< Message
```

## 4. User 用户表

| 字段 | MySQL 类型 | 约束 | 功能 |
|---|---|---|---|
| `UserId` | `BIGINT UNSIGNED` | 主键、自增 | 用户唯一编号 |
| `UserAccount` | `VARCHAR(64)` | 非空、唯一 | 登录账号 |
| `UserPassword` | `VARCHAR(255)` | 非空 | 密码哈希 |
| `Nickname` | `VARCHAR(64)` | 可空 | 用户昵称 |
| `AvatarUrl` | `VARCHAR(512)` | 可空 | 头像资源地址 |
| `Status` | `TINYINT UNSIGNED` | 非空 | 用户状态 |
| `CreatedAt` | `DATETIME` | 非空 | 注册时间 |
| `UpdatedAt` | `DATETIME` | 非空 | 最近更新时间 |

### 用户状态

| 状态值 | C++ 枚举 | 含义 |
|---:|---|---|
| `0` | `db::UserStatus::kNormal` | 正常 |
| `1` | `db::UserStatus::kFrozen` | 冻结 |
| `2` | `db::UserStatus::kDeleted` | 已注销 |

密码只能保存 Argon2id、bcrypt 等算法生成的哈希，不能保存明文密码。用户注销优先修改 `Status`，不要直接删除用户行，否则会破坏历史消息关系。

## 5. UserFriend 联系人表

| 字段 | MySQL 类型 | 约束 | 功能 |
|---|---|---|---|
| `Id` | `BIGINT UNSIGNED` | 主键、自增 | 关系记录编号 |
| `UserId` | `BIGINT UNSIGNED` | 非空、索引 | 当前用户 |
| `FriendId` | `BIGINT UNSIGNED` | 非空、索引 | 对方用户 |
| `Status` | `TINYINT UNSIGNED` | 非空 | 关系状态 |
| `Remark` | `VARCHAR(64)` | 可空 | `UserId` 对 `FriendId` 设置的备注 |
| `CreatedAt` | `DATETIME` | 非空 | 创建时间 |
| `UpdatedAt` | `DATETIME` | 非空 | 更新时间 |

### 联系人状态

| 状态值 | C++ 枚举 | 含义 |
|---:|---|---|
| `0` | `db::FriendStatus::kPending` | 等待处理 |
| `1` | `db::FriendStatus::kAccepted` | 已成为联系人 |
| `2` | `db::FriendStatus::kRejected` | 已拒绝 |
| `3` | `db::FriendStatus::kDeleted` | 已删除 |
| `4` | `db::FriendStatus::kBlocked` | 已拉黑 |

### 联系人存储规则

好友关系采用有方向的记录，便于保存每个用户自己的备注和拉黑状态。

用户 1001 向用户 1002 发起申请时：

```text
UserId=1001, FriendId=1002, Status=0
```

用户 1002 同意后，应在同一事务中：

1. 将 `1001 -> 1002` 更新为 `Status=1`。
2. 插入或恢复 `1002 -> 1001`，并设置为 `Status=1`。

`UserId` 与 `FriendId` 不能相同，二者组合唯一。

## 6. Conversation 会话表

| 字段 | MySQL 类型 | 约束 | 功能 |
|---|---|---|---|
| `ConversationId` | `BIGINT UNSIGNED` | 主键、自增 | 会话唯一编号 |
| `ConversationType` | `TINYINT UNSIGNED` | 非空 | 私聊或群聊 |
| `ConversationName` | `VARCHAR(64)` | 可空 | 群聊名称 |
| `OwnerId` | `BIGINT UNSIGNED` | 可空、索引 | 群主编号，私聊为空 |
| `PrivateUserLowId` | `BIGINT UNSIGNED` | 可空、联合唯一 | 私聊中编号较小的用户 |
| `PrivateUserHighId` | `BIGINT UNSIGNED` | 可空、联合唯一 | 私聊中编号较大的用户 |
| `AvatarUrl` | `VARCHAR(512)` | 可空 | 群头像地址 |
| `Status` | `TINYINT UNSIGNED` | 非空 | 会话状态 |
| `CreatedAt` | `DATETIME` | 非空 | 创建时间 |
| `UpdatedAt` | `DATETIME` | 非空 | 更新时间 |

### 会话类型

| 类型值 | C++ 枚举 | 含义 |
|---:|---|---|
| `1` | `db::ConversationType::kPrivate` | 私聊 |
| `2` | `db::ConversationType::kGroup` | 群聊 |

### 会话状态

| 状态值 | C++ 枚举 | 含义 |
|---:|---|---|
| `0` | `db::ConversationStatus::kNormal` | 正常 |
| `1` | `db::ConversationStatus::kDissolved` | 已解散 |
| `2` | `db::ConversationStatus::kDisabled` | 已禁用 |

### 私聊唯一性

创建私聊前，将两个用户编号排序：

```text
PrivateUserLowId  = min(user_a, user_b)
PrivateUserHighId = max(user_a, user_b)
```

数据库通过这两个字段的联合唯一索引保证同一对用户只有一个私聊会话。私聊创建后，仍需在 `ConversationMember` 中分别写入两名用户。

群聊的两个 `PrivateUser*` 字段必须为空，`OwnerId` 应设置为创建者。

## 7. ConversationMember 会话成员表

| 字段 | MySQL 类型 | 约束 | 功能 |
|---|---|---|---|
| `ConversationId` | `BIGINT UNSIGNED` | 联合主键 | 会话编号 |
| `UserId` | `BIGINT UNSIGNED` | 联合主键、索引 | 成员编号 |
| `Role` | `TINYINT UNSIGNED` | 非空 | 成员角色 |
| `MuteStatus` | `TINYINT UNSIGNED` | 非空 | 禁言状态 |
| `LastReadMessageId` | `BIGINT UNSIGNED` | 可空、索引 | 最后已读消息编号 |
| `JoinedAt` | `DATETIME` | 非空 | 加入时间 |
| `LeftAt` | `DATETIME` | 可空 | 退出时间，空表示仍在会话中 |

### 成员角色

| 角色值 | C++ 枚举 | 含义 |
|---:|---|---|
| `0` | `db::MemberRole::kMember` | 普通成员 |
| `1` | `db::MemberRole::kAdministrator` | 管理员 |
| `2` | `db::MemberRole::kOwner` | 群主 |

私聊的两名成员使用普通成员角色。群聊创建者需要同时满足：

```text
Conversation.OwnerId = 创建者UserId
ConversationMember.Role = 2
```

成员退出时优先写入 `LeftAt`，保留其历史成员记录；重新加入时清空 `LeftAt` 并更新 `JoinedAt`。

## 8. Message 消息表

| 字段 | MySQL 类型 | 约束 | 功能 |
|---|---|---|---|
| `MessageId` | `BIGINT UNSIGNED` | 主键、自增 | 服务端消息编号 |
| `ConversationId` | `BIGINT UNSIGNED` | 非空、联合索引 | 所属会话 |
| `SenderId` | `BIGINT UNSIGNED` | 非空、索引 | 发送者编号 |
| `ClientMessageId` | `CHAR(36)` | 非空、联合唯一 | 客户端生成的 UUID |
| `MessageType` | `TINYINT UNSIGNED` | 非空 | 消息内容类型 |
| `Content` | `TEXT` | 非空 | 文本或媒体元数据 |
| `ReplyMessageId` | `BIGINT UNSIGNED` | 可空 | 被回复的消息 |
| `Status` | `TINYINT UNSIGNED` | 非空 | 正常、撤回或删除 |
| `SendTime` | `DATETIME(3)` | 非空 | 服务端接收时间，精确到毫秒 |

### 消息内容类型

| 类型值 | C++ 枚举 | 含义 |
|---:|---|---|
| `1` | `db::MessageContentType::kText` | 文本 |
| `2` | `db::MessageContentType::kImage` | 图片 |
| `3` | `db::MessageContentType::kAudio` | 语音 |
| `4` | `db::MessageContentType::kVideo` | 视频 |
| `5` | `db::MessageContentType::kFile` | 文件 |
| `6` | `db::MessageContentType::kSystem` | 系统消息 |

`Message.SenderId + Message.ClientMessageId` 组成唯一索引，保证客户端网络重试不会重复保存同一条消息。

历史消息按照以下方式分页，避免使用数据量越大越慢的 `OFFSET`：

```sql
SELECT *
FROM `Message`
WHERE `ConversationId` = ? AND `MessageId` < ?
ORDER BY `MessageId` DESC
LIMIT ?;
```

## 9. 核心业务事务

### 创建私聊

1. 验证两名用户存在且没有被禁用。
2. 对两个用户编号排序。
3. 查询或插入唯一的私聊 `Conversation`。
4. 向 `ConversationMember` 写入两名用户。
5. 提交事务后返回 `ConversationId`。

### 创建群聊

1. 插入 `ConversationType=2` 的会话。
2. 设置 `OwnerId`、群名称和群头像。
3. 将创建者写入 `ConversationMember`，角色为群主。
4. 将初始群成员写入 `ConversationMember`。
5. 全部成功后提交事务。

### 发送消息

1. 根据登录会话取得真实 `SenderId`，不能相信客户端上传的发送者编号。
2. 检查发送者属于该会话、没有退出且没有被禁言。
3. 根据 `ClientMessageId` 去重。
4. 写入 `Message` 并取得服务端 `MessageId`。
5. 提交事务后向会话中的在线成员推送消息。
6. 离线成员下次登录后按照 `LastReadMessageId` 拉取消息。

## 10. 与网络协议的映射

协议操作类型与数据库消息内容类型不是同一个概念。

| 定义 | 示例 | 含义 |
|---|---|---|
| `MessageType` | `kMessageSendRequest` | 客户端正在执行什么网络操作 |
| `db::MessageContentType` | `kText` | 数据库中保存的消息内容是什么类型 |

协议 JSON 使用 `snake_case`，数据库使用固定的 PascalCase 字段。业务层负责转换：

| 协议 JSON | 数据库字段 |
|---|---|
| `conversation_id` | `ConversationId` |
| 登录会话中的用户编号 | `SenderId` |
| `client_message_id` | `ClientMessageId` |
| `message_type` | `MessageType` |
| `content` | `Content` |
| `reply_message_id` | `ReplyMessageId` |

客户端不得直接指定或覆盖以下字段：

- `UserId`
- `SenderId`
- `MessageId`
- `SendTime`
- `OwnerId`，群聊创建之外的操作必须经过权限校验
- `Role`，只有群主或管理员可以修改

## 11. 在线状态与数据库的边界

在线连接属于服务器临时状态，不写入上述五张业务表。

```text
ChatServer.sessions_      Socket -> ClientSession
ChatServer.online_users_  UserId -> Socket
```

用户断线或服务器重启后，这些状态自然消失。以后部署多台聊天服务器时，可以使用 Redis 保存跨服务器在线路由，但 MySQL 仍负责持久化用户、联系人、会话和消息。

## 12. 一致性规则

- 账号必须唯一，密码只能保存哈希。
- 好友关系不能指向自己。
- 接受好友请求时，两条方向记录必须在一个事务中更新。
- 同一对用户只能有一个私聊会话。
- 私聊必须有两个有效成员。
- 群聊必须有且只有一个群主。
- 发送者必须是当前有效会话成员。
- 客户端消息 UUID 必须参与去重。
- 删除用户、会话和消息优先采用状态字段或退出时间，不直接物理删除历史数据。
- 所有外部输入必须通过 MySQL 预处理语句绑定，不能拼接到 SQL 中。

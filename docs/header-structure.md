# Header 目录划分

`server_header` 按数据库、协议和服务器三个区域拆分。业务类一类一文件，公共类型也独立放置，避免一个头文件修改后导致所有模块重新编译。

```text
server_header/
├── database/
│   ├── raw_row.h
│   ├── database_config.h
│   ├── database_schema.h
│   ├── manager_sql.h
│   ├── manage_user.h
│   ├── manage_user_friend.h
│   ├── manage_conversation.h
│   ├── manage_conversation_member.h
│   ├── manage_message.h
│   └── database_managers.h
├── protocol/
│   ├── protocol_constants.h
│   ├── protocol_types.h
│   └── protocol_codec.h
├── server/
│   ├── client_session.h
│   └── chat_server.h
├── database_schema.h
├── manager_sql.h
├── protocol.h
└── chat_server.h
```

## database

| 头文件 | 职责 |
|---|---|
| `raw_row.h` | 定义通用的一行数据库记录 `RawRow` |
| `database_config.h` | 定义 MySQL 地址、账号、端口和数据库名 |
| `database_schema.h` | 集中定义表名、字段名和数据库状态枚举 |
| `manager_sql.h` | 定义数据库抽象父类及通用底层执行接口，只前置声明 MySQL 连接结构 |
| `manage_user.h` | 声明 `User` 表管理类 |
| `manage_user_friend.h` | 声明 `UserFriend` 表管理类 |
| `manage_conversation.h` | 声明 `Conversation` 表管理类 |
| `manage_conversation_member.h` | 声明 `ConversationMember` 表管理类 |
| `manage_message.h` | 声明 `Message` 表管理类 |
| `database_managers.h` | 用前置声明组合服务器需要的五个管理器指针，不引入 MySQL 头文件 |

## protocol

| 头文件 | 职责 |
|---|---|
| `protocol_constants.h` | 定义魔数、版本、包头长度和包体上限 |
| `protocol_types.h` | 定义消息类型、状态码、包头和协议包结构 |
| `protocol_codec.h` | 声明编码、解码和包头校验函数 |

## server

| 头文件 | 职责 |
|---|---|
| `client_session.h` | 保存一个客户端的连接、登录、收发缓冲和心跳状态 |
| `chat_server.h` | 声明 epoll、线程池、会话管理和业务分发接口 |

## 包含规则

- 新代码只包含真正需要的细分头文件，例如 `#include "database/manage_user.h"`。
- MySQL 的完整头文件应由数据库 `.cc` 文件包含，不再扩散到公共头文件。
- `server/chat_server.h` 只通过前置声明持有数据库管理器指针，因此网络模块不依赖 MySQL 开发头文件。
- 顶层四个头文件是旧代码兼容入口，现有 `#include "protocol.h"` 等写法仍然有效。
- 数据库开发主要修改 `server_header/database/`；网络协议开发主要修改 `server_header/protocol/` 和 `server_header/server/`，可以减少两人同时修改同一文件产生的冲突。

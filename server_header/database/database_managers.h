#ifndef KINCHAT_HEADER_DATABASE_DATABASE_MANAGERS_H_
#define KINCHAT_HEADER_DATABASE_DATABASE_MANAGERS_H_

class ManageConversation;
class ManageConversationMember;
class ManageMessage;
class ManageUser;
class ManageUserFriend;

// ChatServer 使用的数据库表管理器集合。
// ChatServer 不拥有这些对象，调用方必须保证它们比 ChatServer 存活更久。
struct DatabaseManagers {
  ManageUser* users = nullptr;
  ManageUserFriend* user_friends = nullptr;
  ManageConversation* conversations = nullptr;
  ManageConversationMember* conversation_members = nullptr;
  ManageMessage* messages = nullptr;
};

#endif  // KINCHAT_HEADER_DATABASE_DATABASE_MANAGERS_H_

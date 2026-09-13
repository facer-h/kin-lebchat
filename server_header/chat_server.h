#ifndef CHAT_SERVER_H_
#define CHAT_SERVER_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "manager_sql.h"
#include "protocol.h"

// ChatServer 使用的数据库表管理器集合。
// ChatServer 不拥有这些对象，调用方必须保证它们比 ChatServer 存活更久。
struct DatabaseManagers {
  ManageUser* users = nullptr;
  ManageUserFriend* user_friends = nullptr;
  ManageConversation* conversations = nullptr;
  ManageConversationMember* conversation_members = nullptr;
  ManageMessage* messages = nullptr;
};

class ChatServer {
 public:
  /*
   * 参数：database_managers，服务器需要使用的五个数据表管理器。
   * 功能：保存数据库管理器引用，初始化服务器的停止和连接状态。
   */
  explicit ChatServer(DatabaseManagers database_managers);

  /*
   * 参数：无。
   * 功能：停止服务器、关闭所有连接并等待工作线程退出。
   */
  ~ChatServer();

  ChatServer(const ChatServer&) = delete;
  ChatServer& operator=(const ChatServer&) = delete;

  /*
   * 参数：port，监听端口；bind_address，监听地址，例如 "0.0.0.0"。
   * 功能：创建监听套接字和 epoll，启动线程池并进入事件循环。
   */
  bool Start(std::uint16_t port, const std::string& bind_address);

  /*
   * 参数：无。
   * 功能：通知事件循环和工作线程停止，并关闭服务器持有的套接字。
   */
  void Stop();

  /*
   * 参数：无。
   * 功能：返回服务器当前是否处于运行状态。
   */
  bool IsRunning() const;

 private:
  // 单个客户端的服务端会话状态。
  struct ClientSession {
    int socket_fd = -1;
    std::uint64_t user_id = 0;
    bool authenticated = false;
    std::vector<std::uint8_t> receive_buffer;
    std::mutex send_mutex;
  };

  /*
   * 参数：port，监听端口；bind_address，监听地址。
   * 功能：创建非阻塞监听套接字，完成 bind、listen 和 epoll 注册。
   */
  bool InitializeNetwork(std::uint16_t port, const std::string& bind_address);

  /*
   * 参数：thread_count，线程池中的工作线程数量。
   * 功能：创建工作线程，让线程持续等待并处理任务队列。
   */
  void StartThreadPool(std::size_t thread_count);

  /*
   * 参数：task，需要在线程池中执行的任务。
   * 功能：将任务加入队列，并唤醒一个等待中的工作线程。
   */
  void AddTask(std::function<void()> task);

  /*
   * 参数：无。
   * 功能：工作线程循环，从任务队列中取出任务并执行。
   */
  void WorkerLoop();

  /*
   * 参数：无。
   * 功能：等待并分发 epoll 网络事件，直到服务器停止。
   */
  void EventLoop();

  /*
   * 参数：无。
   * 功能：循环接受新连接，将客户端套接字设为非阻塞并注册到 epoll。
   */
  void AcceptConnections();

  /*
   * 参数：client_socket，发生可读事件的客户端套接字。
   * 功能：读取 TCP 字节流，将数据追加到该客户端的接收缓冲区。
   */
  void HandleReadable(int client_socket);

  /*
   * 参数：client_socket，拥有待解析数据的客户端套接字。
   * 功能：按协议头中的包长度解决粘包和半包，并逐个分发完整协议包。
   */
  void ParseReceivedPackets(int client_socket);

  /*
   * 参数：client_socket，请求来源；packet，已经完成校验和解码的协议包。
   * 功能：根据协议中的消息类型，将请求交给对应的业务处理函数。
   */
  void DispatchPacket(int client_socket, const ProtocolPacket& packet);

  /*
   * 参数：client_socket，目标连接；packet，需要发送的协议包。
   * 功能：编码并完整发送协议包，处理非阻塞套接字的部分写入。
   */
  bool SendPacket(int client_socket, const ProtocolPacket& packet);

  /*
   * 参数：client_socket，需要关闭的客户端套接字。
   * 功能：从 epoll、会话表和在线用户表移除连接，然后关闭套接字。
   */
  void CloseClient(int client_socket);

  /*
   * 参数：client_socket，已经登录的连接；user_id，登录成功的用户编号。
   * 功能：绑定用户和连接；同一用户重复登录时执行协议约定的处理策略。
   */
  bool BindAuthenticatedUser(int client_socket, std::uint64_t user_id);

  /*
   * 参数：user_id，目标用户编号。
   * 功能：查询目标用户是否在线；在线时返回套接字，不在线时返回 -1。
   */
  int FindOnlineUser(std::uint64_t user_id) const;

  /*
   * 参数：client_socket，请求来源；packet，注册请求协议包。
   * 功能：校验账号和密码字段，创建用户并返回注册结果。
   */
  void HandleUserRegistration(int client_socket, const ProtocolPacket& packet);

  /*
   * 参数：client_socket，请求来源；packet，登录请求协议包。
   * 功能：验证账号和密码，建立登录会话并返回访问令牌或登录结果。
   */
  void HandleUserLogin(int client_socket, const ProtocolPacket& packet);

  /*
   * 参数：client_socket，请求来源；packet，用户信息请求协议包。
   * 功能：查询或更新经过授权的用户资料，并返回操作结果。
   */
  void HandleUserInfoRequest(int client_socket, const ProtocolPacket& packet);

  /*
   * 参数：client_socket，请求来源；packet，好友操作协议包。
   * 功能：处理添加、同意、拒绝、删除好友以及获取好友列表请求。
   */
  void HandleFriendRequest(int client_socket, const ProtocolPacket& packet);

  /*
   * 参数：client_socket，请求来源；packet，会话操作协议包。
   * 功能：处理创建会话、获取会话列表和获取会话成员等请求。
   */
  void HandleConversationRequest(int client_socket,
                                 const ProtocolPacket& packet);

  /*
   * 参数：client_socket，请求来源；packet，私聊消息协议包。
   * 功能：验证会话成员身份、保存消息，并向在线接收者转发消息。
   */
  void HandlePrivateMessage(int client_socket, const ProtocolPacket& packet);

  /*
   * 参数：client_socket，请求来源；packet，群聊消息协议包。
   * 功能：验证群成员身份、保存消息，并向在线群成员转发消息。
   */
  void HandleGroupMessage(int client_socket, const ProtocolPacket& packet);

  DatabaseManagers database_managers_;

  int listen_fd_ = -1;
  int epoll_fd_ = -1;
  std::atomic<bool> running_ = false;

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::condition_variable tasks_cv_;
  std::mutex tasks_mutex_;

  std::unordered_map<int, std::shared_ptr<ClientSession>> sessions_;
  std::unordered_map<std::uint64_t, int> online_users_;
  mutable std::mutex sessions_mutex_;
};

#endif  // CHAT_SERVER_H_

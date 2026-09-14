#ifndef KINCHAT_HEADER_SERVER_CHAT_SERVER_H_
#define KINCHAT_HEADER_SERVER_CHAT_SERVER_H_

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

#include "database/database_managers.h"
#include "protocol/protocol_codec.h"
#include "server/client_session.h"

// 即时通信服务器：负责网络连接、协议分发、在线状态和业务调度。
class ChatServer {
 public:
  /*
   * 参数：database_managers，服务器需要使用的五个数据表管理器。
   * 功能：保存数据库管理器引用，初始化服务器的停止和连接状态。
   */
  explicit ChatServer(DatabaseManagers database_managers);

  /* 参数：无。功能：停止服务器、关闭连接并等待工作线程退出。 */
  ~ChatServer();

  ChatServer(const ChatServer&) = delete;
  ChatServer& operator=(const ChatServer&) = delete;

  /*
   * 参数：port，监听端口；bind_address，监听地址，例如 "0.0.0.0"。
   * 功能：创建监听套接字和 epoll，启动线程池并进入事件循环。
   */
  bool Start(std::uint16_t port, const std::string& bind_address);
  /* 参数：无。功能：通知事件循环和工作线程停止并关闭套接字。 */
  void Stop();
  /* 参数：无。功能：返回服务器当前是否处于运行状态。 */
  bool IsRunning() const;

 private:
  // 兼容原先的 ChatServer::ClientSession 名称，实际定义已拆到独立头文件。
  using ClientSession = ::ClientSession;

  /*
   * 参数：port，监听端口；bind_address，监听地址。
   * 功能：创建非阻塞监听套接字，完成 bind、listen 和 epoll 注册。
   */
  bool InitializeNetwork(std::uint16_t port, const std::string& bind_address);
  /*
   * 参数：thread_count，工作线程数量。
   * 功能：创建线程池，让线程持续等待并处理任务。
   */
  void StartThreadPool(std::size_t thread_count);
  /* 参数：task，待执行任务。功能：加入任务队列并唤醒一个工作线程。 */
  void AddTask(std::function<void()> task);
  /* 参数：无。功能：从任务队列中循环取出并执行任务。 */
  void WorkerLoop();
  /* 参数：无。功能：等待并分发 epoll 事件，直到服务器停止。 */
  void EventLoop();
  /* 参数：无。功能：接受新连接并将非阻塞套接字注册到 epoll。 */
  void AcceptConnections();

  /*
   * 参数：client_socket，发生可读事件的客户端套接字。
   * 功能：读取 TCP 字节流并追加到客户端接收缓冲区。
   */
  void HandleReadable(int client_socket);
  /*
   * 参数：client_socket，发生可写事件的客户端套接字。
   * 功能：继续发送尚未写完的数据并维护 EPOLLOUT 事件。
   */
  void HandleWritable(int client_socket);
  /*
   * 参数：client_socket，拥有待解析数据的客户端套接字。
   * 功能：处理粘包和半包并逐个分发完整协议包。
   */
  void ParseReceivedPackets(int client_socket);
  /*
   * 参数：client_socket，请求来源；packet，已校验并解码的协议包。
   * 功能：根据消息类型将请求交给对应业务处理函数。
   */
  void DispatchPacket(int client_socket, const ProtocolPacket& packet);
  /*
   * 参数：client_socket，目标连接；packet，待发送协议包。
   * 功能：编码并发送协议包，处理非阻塞套接字的部分写入。
   */
  bool SendPacket(int client_socket, const ProtocolPacket& packet);
  /*
   * 参数：client_socket，待关闭套接字。
   * 功能：从 epoll、会话表和在线用户表移除连接并关闭套接字。
   */
  void CloseClient(int client_socket);

  /*
   * 参数：client_socket，已登录连接；user_id，登录成功的用户编号。
   * 功能：绑定用户和连接，并处理同一用户重复登录。
   */
  bool BindAuthenticatedUser(int client_socket, std::uint64_t user_id);
  /*
   * 参数：user_id，目标用户编号。
   * 功能：在线时返回套接字，不在线时返回 -1。
   */
  int FindOnlineUser(std::uint64_t user_id) const;
  /* 参数：无。功能：关闭超过心跳超时时间的连接。 */
  void CheckIdleConnections();

  /*
   * 参数：client_socket，请求来源；packet，心跳请求。
   * 功能：刷新活跃时间并返回相同 sequence_id 的心跳响应。
   */
  void HandleHeartbeat(int client_socket, const ProtocolPacket& packet);
  /*
   * 参数：client_socket，请求来源；packet，注册请求。
   * 功能：校验账号和密码，创建用户并返回结果。
   */
  void HandleUserRegistration(int client_socket, const ProtocolPacket& packet);
  /*
   * 参数：client_socket，请求来源；packet，登录请求。
   * 功能：验证账号和密码，建立登录会话并返回结果。
   */
  void HandleUserLogin(int client_socket, const ProtocolPacket& packet);
  /*
   * 参数：client_socket，请求来源；packet，用户资料请求。
   * 功能：查询或更新经过授权的用户资料。
   */
  void HandleUserInfoRequest(int client_socket, const ProtocolPacket& packet);
  /*
   * 参数：client_socket，请求来源；packet，好友操作请求。
   * 功能：处理添加、回复、删除好友和好友列表请求。
   */
  void HandleFriendRequest(int client_socket, const ProtocolPacket& packet);
  /*
   * 参数：client_socket，请求来源；packet，会话操作请求。
   * 功能：处理创建、查询和修改会话等请求。
   */
  void HandleConversationRequest(int client_socket,
                                 const ProtocolPacket& packet);
  /*
   * 参数：client_socket，请求来源；packet，会话成员操作请求。
   * 功能：处理添加、移除、退出和查询会话成员。
   */
  void HandleConversationMemberRequest(int client_socket,
                                       const ProtocolPacket& packet);
  /*
   * 参数：client_socket，请求来源；packet，消息发送请求。
   * 功能：验证成员身份、保存消息并向会话中的在线成员推送。
   */
  void HandleMessageSend(int client_socket, const ProtocolPacket& packet);

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

#endif  // KINCHAT_HEADER_SERVER_CHAT_SERVER_H_

#ifndef KINCHAT_HEADER_SERVER_CLIENT_SESSION_H_
#define KINCHAT_HEADER_SERVER_CLIENT_SESSION_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

// 一个 TCP 客户端在服务器内存中的连接和登录状态。
struct ClientSession {
  int socket_fd = -1;
  std::uint64_t user_id = 0;
  bool authenticated = false;
  std::vector<std::uint8_t> receive_buffer;
  std::vector<std::uint8_t> send_buffer;
  std::size_t send_offset = 0;
  std::chrono::steady_clock::time_point last_active_time =
      std::chrono::steady_clock::now();
  std::mutex send_mutex;
};

#endif  // KINCHAT_HEADER_SERVER_CLIENT_SESSION_H_

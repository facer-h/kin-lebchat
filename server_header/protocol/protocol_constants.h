#ifndef KINCHAT_HEADER_PROTOCOL_PROTOCOL_CONSTANTS_H_
#define KINCHAT_HEADER_PROTOCOL_PROTOCOL_CONSTANTS_H_

#include <cstdint>

// 协议中的所有整数在线上传输时统一使用网络字节序（大端序）。
inline constexpr std::uint32_t kProtocolMagic = 0x43484154;  // 协议标识
inline constexpr std::uint16_t kProtocolVersion = 1;    // 协议版本号
inline constexpr std::uint16_t kProtocolHeaderSize = 24;  // 协议头大小
inline constexpr std::uint32_t kMaxProtocolBodySize = 1024 * 1024;  // 1 MiB // 协议体最大大小

#endif  // KINCHAT_HEADER_PROTOCOL_PROTOCOL_CONSTANTS_H_

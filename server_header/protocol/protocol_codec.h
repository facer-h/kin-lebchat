#ifndef KINCHAT_HEADER_PROTOCOL_PROTOCOL_CODEC_H_
#define KINCHAT_HEADER_PROTOCOL_PROTOCOL_CODEC_H_

#include <cstdint>
#include <string>
#include <vector>

#include "protocol_types.h"

// TCP 协议包的编码、解码和包头校验工具。
class ProtocolCodec {
 public:
  /*
   * 参数：packet，需要编码的协议包；output，接收编码结果的字节数组。
   * 功能：将包头转换为网络字节序，并追加 UTF-8 JSON 包体。
   */
  static bool Encode(const ProtocolPacket& packet,
                     std::vector<std::uint8_t>* output);

  /*
   * 参数：buffer，累计接收缓冲区；packet，接收解码结果；
   *       error，接收协议错误，可以为空。
   * 功能：从 TCP 字节流中解码一个完整包；成功后从 buffer 移除该包。
   */
  static DecodeResult DecodeOne(std::vector<std::uint8_t>* buffer,
                                ProtocolPacket* packet, std::string* error);

  /*
   * 参数：header，待校验包头；error，接收错误，可以为空。
   * 功能：检查魔数、版本、包头长度、消息类型和包体长度是否合法。
   */
  static bool ValidateHeader(const PacketHeader& header, std::string* error);
};

#endif  // KINCHAT_HEADER_PROTOCOL_PROTOCOL_CODEC_H_

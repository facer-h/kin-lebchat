#include "protocol.h"
#include <limits>

namespace {

void SetError(std::string* error, const std::string& message) {
  if (error != nullptr) {
    *error = message;
  }
}

void AppendUint16(std::uint16_t value, std::vector<std::uint8_t>* output) {
  output->push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
  output->push_back(static_cast<std::uint8_t>(value & 0xff));
}

void AppendUint32(std::uint32_t value, std::vector<std::uint8_t>* output) {
  output->push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
  output->push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
  output->push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
  output->push_back(static_cast<std::uint8_t>(value & 0xff));
}

void AppendUint64(std::uint64_t value, std::vector<std::uint8_t>* output) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    output->push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
  }
}

std::uint16_t ReadUint16(const std::uint8_t* data) {
  return (static_cast<std::uint16_t>(data[0]) << 8) |
         static_cast<std::uint16_t>(data[1]);
}

std::uint32_t ReadUint32(const std::uint8_t* data) {
  return (static_cast<std::uint32_t>(data[0]) << 24) |
         (static_cast<std::uint32_t>(data[1]) << 16) |
         (static_cast<std::uint32_t>(data[2]) << 8) |
         static_cast<std::uint32_t>(data[3]);
}

std::uint64_t ReadUint64(const std::uint8_t* data) {
  std::uint64_t value = 0;
  for (int index = 0; index < 8; ++index) {
    value = (value << 8) | static_cast<std::uint64_t>(data[index]);
  }
  return value;
}

bool IsKnownMessageType(MessageType type) {
  switch (type) {
    case MessageType::kHeartbeatRequest:
    case MessageType::kHeartbeatResponse:
    case MessageType::kErrorResponse:
    case MessageType::kRegisterRequest:
    case MessageType::kRegisterResponse:
    case MessageType::kLoginRequest:
    case MessageType::kLoginResponse:
    case MessageType::kLogoutRequest:
    case MessageType::kLogoutResponse:
    case MessageType::kUserInfoRequest:
    case MessageType::kUserInfoResponse:
    case MessageType::kUserSearchRequest:
    case MessageType::kUserSearchResponse:
    case MessageType::kFriendAddRequest:
    case MessageType::kFriendAddResponse:
    case MessageType::kFriendReplyRequest:
    case MessageType::kFriendReplyResponse:
    case MessageType::kFriendDeleteRequest:
    case MessageType::kFriendDeleteResponse:
    case MessageType::kFriendListRequest:
    case MessageType::kFriendListResponse:
    case MessageType::kFriendRequestPush:
    case MessageType::kFriendStatusPush:
    case MessageType::kConversationCreateRequest:
    case MessageType::kConversationCreateResponse:
    case MessageType::kConversationListRequest:
    case MessageType::kConversationListResponse:
    case MessageType::kConversationMemberRequest:
    case MessageType::kConversationMemberResponse:
    case MessageType::kConversationUpdateRequest:
    case MessageType::kConversationUpdateResponse:
    case MessageType::kConversationLeaveRequest:
    case MessageType::kConversationLeaveResponse:
    case MessageType::kConversationMemberAddRequest:
    case MessageType::kConversationMemberAddResponse:
    case MessageType::kConversationMemberRemoveRequest:
    case MessageType::kConversationMemberRemoveResponse:
    case MessageType::kMessageSendRequest:
    case MessageType::kMessageSendResponse:
    case MessageType::kMessagePush:
    case MessageType::kMessageHistoryRequest:
    case MessageType::kMessageHistoryResponse:
    case MessageType::kMessageAckRequest:
    case MessageType::kMessageAckResponse:
      return true;
    case MessageType::kUnknown:
      return false;
  }
  return false;
}

}  // namespace

bool ProtocolCodec::Encode(const ProtocolPacket& packet,
                           std::vector<std::uint8_t>* output) {
  if (output == nullptr) {
    return false;
  }
  if (packet.body.size() > kMaxProtocolBodySize ||
      packet.body.size() > std::numeric_limits<std::uint32_t>::max()) {
    output->clear();
    return false;
  }

  PacketHeader header = packet.header;
  header.body_size = static_cast<std::uint32_t>(packet.body.size());
  if (!ValidateHeader(header, nullptr)) {
    output->clear();
    return false;
  }

  output->clear();
  output->reserve(kProtocolHeaderSize + packet.body.size());
  AppendUint32(header.magic, output);
  AppendUint16(header.version, output);
  AppendUint16(header.header_size, output);
  AppendUint16(static_cast<std::uint16_t>(header.message_type), output);
  AppendUint16(header.flags, output);
  AppendUint32(header.body_size, output);
  AppendUint64(header.sequence_id, output);
  output->insert(output->end(), packet.body.begin(), packet.body.end());
  return true;
}

DecodeResult ProtocolCodec::DecodeOne(std::vector<std::uint8_t>* buffer,
                                      ProtocolPacket* packet,
                                      std::string* error) {
  if (buffer == nullptr || packet == nullptr) {
    SetError(error, "buffer and packet must not be null");
    return DecodeResult::kInvalidPacket;
  }
  if (buffer->size() < kProtocolHeaderSize) {
    return DecodeResult::kNeedMoreData;
  }

  const std::uint8_t* data = buffer->data();
  PacketHeader header;
  header.magic = ReadUint32(data);
  header.version = ReadUint16(data + 4);
  header.header_size = ReadUint16(data + 6);
  header.message_type = static_cast<MessageType>(ReadUint16(data + 8));
  header.flags = ReadUint16(data + 10);
  header.body_size = ReadUint32(data + 12);
  header.sequence_id = ReadUint64(data + 16);

  if (!ValidateHeader(header, error)) {
    return DecodeResult::kInvalidPacket;
  }

  const std::size_t packet_size =
      static_cast<std::size_t>(header.header_size) + header.body_size;
  if (buffer->size() < packet_size) {
    return DecodeResult::kNeedMoreData;
  }

  ProtocolPacket decoded_packet;
  decoded_packet.header = header;
  decoded_packet.body.assign(
      reinterpret_cast<const char*>(data + header.header_size),
      header.body_size);
  *packet = std::move(decoded_packet);
  buffer->erase(buffer->begin(), buffer->begin() + packet_size);
  if (error != nullptr) {
    error->clear();
  }
  return DecodeResult::kSuccess;
}

bool ProtocolCodec::ValidateHeader(const PacketHeader& header,
                                   std::string* error) {
  if (header.magic != kProtocolMagic) {
    SetError(error, "invalid protocol magic");
    return false;
  }
  if (header.version != kProtocolVersion) {
    SetError(error, "unsupported protocol version");
    return false;
  }
  if (header.header_size != kProtocolHeaderSize) {
    SetError(error, "invalid protocol header size");
    return false;
  }
  if (!IsKnownMessageType(header.message_type)) {
    SetError(error, "unknown message type");
    return false;
  }
  constexpr std::uint16_t kKnownFlags =
      kFlagResponse | kFlagServerPush | kFlagCompressed | kFlagEncrypted;
  if ((header.flags & ~kKnownFlags) != 0) {
    SetError(error, "unknown packet flags");
    return false;
  }
  if (header.body_size > kMaxProtocolBodySize) {
    SetError(error, "protocol body is too large");
    return false;
  }
  if (error != nullptr) {
    error->clear();
  }
  return true;
}

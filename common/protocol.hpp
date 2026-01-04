#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "utils.hpp"

// Enum cho các loại thông điệp
enum class MessageType : uint8_t {
  // Test
  TEST = 0x00,     // Dùng để kiểm tra kết nối
  RESPONSE = 0x01, // Phản hồi chung

  // Register
  REGISTER = 0x10,         // Client gửi yêu cầu đăng ký tài khoản
  REGISTER_SUCCESS = 0x11, // Server phản hồi đăng ký thành công
  REGISTER_FAILURE = 0x12, // Server phản hồi đăng ký thất bại

  // Login
  LOGIN = 0x20,         // Client gửi yêu cầu đăng nhập
  LOGIN_SUCCESS = 0x21, // Server phản hồi đăng nhập thành công
  LOGIN_FAILURE = 0x22, // Server phản hồi đăng nhập thất bại

  // Player list
  REQUEST_PLAYER_LIST = 0x30, // Client yêu cầu danh sách người chơi online
  PLAYER_LIST = 0x31,         // Server gửi danh sách người chơi

  // Game
  GAME_START = 0x40,   // Server thông báo bắt đầu ván cờ
  MOVE = 0x41,         // Client gửi nước đi
  INVALID_MOVE = 0x42, // Server thông báo nước đi không hợp lệ
  GAME_STATUS_UPDATE =
      0x43,         // Server cập nhật trạng thái ván cờ (FEN, lượt đi...)
  GAME_END = 0x44,  // Server thông báo kết thúc ván cờ
  SURRENDER = 0x45, // Client xin đầu hàng

  // Challenge
  CHALLENGE_REQUEST = 0x50, // Client gửi lời mời thách đấu
  CHALLENGE_NOTIFICATION =
      0x51, // Server thông báo có lời mời thách đấu đến người được mời
  CHALLENGE_RESPONSE = 0x52, // Người được mời phản hồi (chấp nhận/từ chối)
  CHALLENGE_ACCEPTED = 0x53, // Server thông báo lời mời được chấp nhận
  CHALLENGE_DECLINED = 0x54, // Server thông báo lời mời bị từ chối

  // Auto match
  AUTO_MATCH_REQUEST = 0x55,  // Client yêu cầu tìm trận tự động
  AUTO_MATCH_FOUND = 0x56,    // Server thông báo đã tìm thấy đối thủ
  AUTO_MATCH_ACCEPTED = 0x57, // Client chấp nhận trận đấu tìm được
  AUTO_MATCH_DECLINED = 0x58, // Client từ chối trận đấu tìm được
  AUTO_MATCH_DECLINED_NOTIFICATION =
      0x59, // Server thông báo đối thủ đã từ chối

  // Additional
  CHALLENGE_ERROR = 0x5B, // Lỗi trong quá trình thách đấu
  GAME_LOG = 0x46         // Ghi log ván đấu
};

// Cấu trúc gói tin cơ bản
// +---------+-----------+------------------+
// |  type   |  length   |     payload      |
// | 1 byte  |  2 bytes  |   length bytes   |
// +---------+-----------+------------------+
struct Packet {
  MessageType type;
  uint16_t length;
  std::vector<uint8_t> payload;

  std::vector<uint8_t> serialize() const {
    std::vector<uint8_t> packet;
    packet.push_back(static_cast<uint8_t>(type));
    packet.push_back(static_cast<uint8_t>((length >> 8) & 0xFF)); // High byte
    packet.push_back(static_cast<uint8_t>(length & 0xFF));        // Low byte
    packet.insert(packet.end(), payload.begin(), payload.end());
    return packet;
  }
};

#endif // PROTOCOL_HPP
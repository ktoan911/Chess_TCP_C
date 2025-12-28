#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <vector>
#include <string>

#include "utils.hpp"

// Enum cho các loại thông điệp
enum class MessageType : uint8_t
{
    // Test
    TEST = 0x00,
    RESPONSE = 0x01,

    // Register
    REGISTER = 0x10,
    REGISTER_SUCCESS = 0x11,
    REGISTER_FAILURE = 0x12,

    // Login
    LOGIN = 0x20,
    LOGIN_SUCCESS = 0x21,
    LOGIN_FAILURE = 0x22,

    // Player list
    REQUEST_PLAYER_LIST = 0x30,
    PLAYER_LIST = 0x31,

    // Game
    GAME_START = 0x40,
    MOVE = 0x41,
    INVALID_MOVE = 0x42,
    GAME_STATUS_UPDATE = 0x43,
    GAME_END = 0x44,
    SURRENDER = 0x45,

    // Challenge
    CHALLENGE_REQUEST = 0x50,
    CHALLENGE_NOTIFICATION = 0x51,
    CHALLENGE_RESPONSE = 0x52,
    CHALLENGE_ACCEPTED = 0x53,
    CHALLENGE_DECLINED = 0x54,
    
    // Auto match
    AUTO_MATCH_REQUEST = 0x55,
    AUTO_MATCH_FOUND = 0x56,
    AUTO_MATCH_ACCEPTED = 0x57,
    AUTO_MATCH_DECLINED = 0x58,
    AUTO_MATCH_DECLINED_NOTIFICATION = 0x59,
    
    // Additional
    CHALLENGE_ERROR = 0x5B,
    GAME_LOG = 0x46
};

// Cấu trúc gói tin cơ bản
// +---------+-----------+------------------+
// |  type   |  length   |     payload      |
// | 1 byte  |  2 bytes  |   length bytes   |
// +---------+-----------+------------------+
struct Packet
{
    MessageType type;
    uint16_t length;
    std::vector<uint8_t> payload;

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> packet;
        packet.push_back(static_cast<uint8_t>(type));
        packet.push_back(static_cast<uint8_t>((length >> 8) & 0xFF)); // High byte
        packet.push_back(static_cast<uint8_t>(length & 0xFF));        // Low byte
        packet.insert(packet.end(), payload.begin(), payload.end());
        return packet;
    }
};

#endif // PROTOCOL_HPP
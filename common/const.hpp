#ifndef CONST_HPP
#define CONST_HPP

#include <string>
#include <cstdint>

namespace Const
{
    // Network constants
    const uint16_t SERVER_PORT = 8088;
    const std::string SERVER_IP = "127.0.0.1";
    const uint16_t BUFFER_SIZE = 1024;
    const uint8_t BACKLOG = 5;

    // Game constants
    const uint16_t DEFAULT_ELO = 1200;
    const uint16_t DEFAULT_TIME = 300; // 5 minutes
    const uint16_t DEFAULT_INCREMENT = 5; // 5 seconds

    // Matchmaking constants
    const uint16_t ELO_THRESHOLD = 300;
}

enum class GameResult
{
    NONE,
    WHITE_WIN,
    BLACK_WIN,
    DRAW
};

#endif // CONST_HPP
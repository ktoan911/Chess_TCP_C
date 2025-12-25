#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>
#include <memory>

#include "utils.hpp"
#include "protocol.hpp"
#include <stdexcept>

constexpr size_t MAX_FIELD_LENGTH = 255;

inline void ensure_available(const std::vector<uint8_t>& payload, size_t pos, size_t n)
{
    if (pos + n > payload.size())
        throw std::runtime_error("payload too small");
}

inline uint8_t read_u8(const std::vector<uint8_t>& payload, size_t &pos)
{
    ensure_available(payload, pos, 1);
    return payload[pos++];
}

inline uint16_t read_u16_be(const std::vector<uint8_t>& payload, size_t &pos)
{
    ensure_available(payload, pos, 2);
    uint16_t v = from_big_endian_16(payload, pos);
    pos += 2;
    return v;
}

inline int64_t read_i64_be(const std::vector<uint8_t>& payload, size_t &pos)
{
    ensure_available(payload, pos, 8);
    int64_t v = 0;
    for (int i = 0; i < 8; ++i)
    {
        v = (v << 8) | payload[pos++];
    }
    return v;
}

inline std::string read_string(const std::vector<uint8_t>& payload, size_t &pos)
{
    uint8_t length = read_u8(payload, pos);
    if (length > MAX_FIELD_LENGTH)
        throw std::runtime_error("field length too large");
    ensure_available(payload, pos, length);
    std::string s(payload.begin() + pos, payload.begin() + pos + length);
    pos += length;
    return s;
}

#pragma region RegisterMessage 
// RegisterMessage 
/*
Send from client to server to register a new user.

Payload structure:
    - uint8_t username_length (1 byte)
    - char[username_length] username (username_length bytes)
*/
struct RegisterMessage
{
    std::string username;

    MessageType getType() const
    {
        return MessageType::REGISTER;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        return payload;
    }

    static RegisterMessage deserialize(const std::vector<uint8_t> &payload)
    {
        RegisterMessage message;
        size_t pos = 0;
        message.username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion RegisterMessage

#pragma region RegisterSuccessMessage 
/*
Send from server to client to notify that the registration was successful.

Payload structure:
    - uint8_t username_length (1 byte)
    - char[username_length] username (username_length bytes)
    - uint16_t elo (2 bytes)
*/
struct RegisterSuccessMessage
{
    std::string username;
    uint16_t elo;

    MessageType getType() const
    {
        return MessageType::REGISTER_SUCCESS;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        std::vector<uint8_t> elo_bytes = to_big_endian_16(elo);
        payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());

        return payload;
    }

    static RegisterSuccessMessage deserialize(const std::vector<uint8_t> &payload)
    {
        RegisterSuccessMessage message;
        size_t pos = 0;
        message.username = read_string(payload, pos);
        message.elo = read_u16_be(payload, pos);
        return message;
    }
};
#pragma endregion RegisterSuccessMessage

#pragma region RegisterFailureMessage 
/*
Send from server to client to notify that the registration was unsuccessful.

Payload structure:
    - uint8_t error_message_length (1 byte)
    - char[error_message_length] error_message (error_message_length bytes)
*/
struct RegisterFailureMessage
{
    std::string error_message;

    MessageType getType() const
    {
        return MessageType::REGISTER_FAILURE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(error_message.size()));
        payload.insert(payload.end(), error_message.begin(), error_message.end());

        return payload;
    }

    static RegisterFailureMessage deserialize(const std::vector<uint8_t> &payload)
    {
        RegisterFailureMessage message;
        size_t pos = 0;
        message.error_message = read_string(payload, pos);
        return message;
    }
};
#pragma endregion RegisterFailureMessage

#pragma region LoginMessage 
/*
Send from client to server to login.

Payload structure:
    - uint8_t username_length (1 byte)
    - char[username_length] username (username_length bytes)
*/
struct LoginMessage
{
    std::string username;

    MessageType getType() const
    {
        return MessageType::LOGIN;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        return payload;
    }

    static LoginMessage deserialize(const std::vector<uint8_t> &payload)
    {
        LoginMessage message;
        size_t pos = 0;
        message.username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion LoginMessage

#pragma region LoginSuccessMessage 
/*
Send from server to client to notify that the login was successful.

Payload structure:
    - uint8_t username_length (1 byte)
    - char[username_length] username (username_length bytes)
    - uint16_t elo (2 bytes)
*/
struct LoginSuccessMessage
{
    std::string username;
    uint16_t elo;

    MessageType getType() const
    {
        return MessageType::LOGIN_SUCCESS;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;
        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        std::vector<uint8_t> elo_bytes = to_big_endian_16(elo);
        payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());
        return payload;
    }

    static LoginSuccessMessage deserialize(const std::vector<uint8_t> &payload)
    {
        LoginSuccessMessage message;
        size_t pos = 0;
        message.username = read_string(payload, pos);
        message.elo = read_u16_be(payload, pos);
        return message;
    }
};

#pragma region LoginFailureMessage 
/*
Send from server to client to notify that the login was unsuccessful.

Payload structure:
    - uint8_t error_message_length (1 byte)
    - char[error_message_length] error_message (error_message_length bytes)
*/
struct LoginFailureMessage
{
    std::string error_message;

    MessageType getType() const
    {
        return MessageType::LOGIN_FAILURE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(error_message.size()));
        payload.insert(payload.end(), error_message.begin(), error_message.end());

        return payload;
    }

    static LoginFailureMessage deserialize(const std::vector<uint8_t> &payload)
    {
        LoginFailureMessage message;
        size_t pos = 0;
        message.error_message = read_string(payload, pos);
        return message;
    }
};
#pragma endregion LoginFailureMessage

#pragma region GameStartMessage 
/*
Send from server to clients to notify that a new game has started.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
    - uint8_t player1_username_length (1 byte)
    - char[player1_username_length] player1_username (player1_username_length bytes)
    - uint8_t player2_username_length (1 byte)
    - char[player2_username_length] player2_username (player2_username_length bytes)
    - uint8_t starting_player_username_length (1 byte)
    - char[starting_player_username_length] starting_player_username (starting_player_username_length bytes)
    - uint8_t fen_length (1 byte)
    - char[fen_length] fen (fen_length bytes)
*/
struct GameStartMessage
{
    std::string game_id;
    std::string player1_username;
    std::string player2_username;
    std::string starting_player_username;
    std::string fen;

    MessageType getType() const
    {
        return MessageType::GAME_START;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        payload.push_back(static_cast<uint8_t>(player1_username.size()));
        payload.insert(payload.end(), player1_username.begin(), player1_username.end());

        payload.push_back(static_cast<uint8_t>(player2_username.size()));
        payload.insert(payload.end(), player2_username.begin(), player2_username.end());

        payload.push_back(static_cast<uint8_t>(starting_player_username.size()));
        payload.insert(payload.end(), starting_player_username.begin(), starting_player_username.end());

        payload.push_back(static_cast<uint8_t>(fen.size()));
        payload.insert(payload.end(), fen.begin(), fen.end());

        return payload;
    }

    static GameStartMessage deserialize(const std::vector<uint8_t> &payload)
    {
        GameStartMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.player1_username = read_string(payload, pos);
        message.player2_username = read_string(payload, pos);
        message.starting_player_username = read_string(payload, pos);
        message.fen = read_string(payload, pos);
        return message;
    }
};
#pragma endregion GameStartMessage

#pragma region MoveMessage 
/*
Send from client to server to make a move.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
    - uint8_t uci_move_length (1 byte)
    - char[uci_move_length] uci_move (uci_move_length bytes)
*/
struct MoveMessage
{
    std::string game_id;
    std::string uci_move;

    MessageType getType() const
    {
        return MessageType::MOVE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        payload.push_back(static_cast<uint8_t>(uci_move.size()));
        payload.insert(payload.end(), uci_move.begin(), uci_move.end());

        return payload;
    }

    static MoveMessage deserialize(const std::vector<uint8_t> &payload)
    {
        MoveMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.uci_move = read_string(payload, pos);
        return message;
    }
};
#pragma endregion MoveMessage

#pragma region InvalidMoveMessage 
/*
Send from server to client to notify that the move was invalid.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
    - uint8_t error_message_length (1 byte)
    - char[error_message_length] error_message (error_message_length bytes)
*/
struct InvalidMoveMessage
{
    std::string game_id;
    std::string error_message;

    MessageType getType() const
    {
        return MessageType::INVALID_MOVE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        payload.push_back(static_cast<uint8_t>(error_message.size()));
        payload.insert(payload.end(), error_message.begin(), error_message.end());

        return payload;
    }

    static InvalidMoveMessage deserialize(const std::vector<uint8_t> &payload)
    {
        InvalidMoveMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.error_message = read_string(payload, pos);
        return message;
    }
};
#pragma endregion InvalidMoveMessage

#pragma region GameStatusUpdateMessage 
/*
Send from server to clients to notify that the game status has been updated.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)

    - uint8_t fen_length (1 byte)
    - char[fen_length] fen (fen_length bytes)

    - uint8_t current_turn_username_length (1 byte)
    - char[current_turn_username_length] current_turn_username (current_turn_username_length bytes)

    - uint8_t is_game_over (1 byte) (0: false, 1: true)

    - uint8_t message_length (1 byte)
    - char[message_length] message (message_length bytes)
*/
struct GameStatusUpdateMessage
{
    std::string game_id;
    std::string fen;
    std::string current_turn_username;
    uint8_t is_game_over;
    std::string message;

    MessageType getType() const
    {
        return MessageType::GAME_STATUS_UPDATE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        payload.push_back(static_cast<uint8_t>(fen.size()));
        payload.insert(payload.end(), fen.begin(), fen.end());

        payload.push_back(static_cast<uint8_t>(current_turn_username.size()));
        payload.insert(payload.end(), current_turn_username.begin(), current_turn_username.end());

        payload.push_back(is_game_over);

        payload.push_back(static_cast<uint8_t>(message.size()));
        payload.insert(payload.end(), message.begin(), message.end());
        
        return payload;
    }

    static GameStatusUpdateMessage deserialize(const std::vector<uint8_t> &payload)
    {
        GameStatusUpdateMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.fen = read_string(payload, pos);
        message.current_turn_username = read_string(payload, pos);
        message.is_game_over = read_u8(payload, pos);
        message.message = read_string(payload, pos);
        return message;
    }
};
#pragma endregion GameStatusUpdateMessage

#pragma region GameEndMessage 
/*
Send from server to clients to notify that the game has ended.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)

    - uint8_t winner_username_length (1 byte)
    - char[winner_username_length] winner_username (winner_username_length bytes)

    - uint8_t reason_length (1 byte)
    - char[reason_length] reason (reason_length bytes)

    - uint16_t half_moves_count (2 bytes)
*/
struct GameEndMessage
{
    std::string game_id;
    std::string winner_username;
    std::string reason;
    uint16_t half_moves_count;

    MessageType getType() const
    {
        return MessageType::GAME_END;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        payload.push_back(static_cast<uint8_t>(winner_username.size()));
        payload.insert(payload.end(), winner_username.begin(), winner_username.end());

        payload.push_back(static_cast<uint8_t>(reason.size()));
        payload.insert(payload.end(), reason.begin(), reason.end());

        std::vector<uint8_t> full_moves_count_bytes = to_big_endian_16(half_moves_count);
        payload.insert(payload.end(), full_moves_count_bytes.begin(), full_moves_count_bytes.end());

        return payload;
    }

    static GameEndMessage deserialize(const std::vector<uint8_t> &payload)
    {
        GameEndMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.winner_username = read_string(payload, pos);
        message.reason = read_string(payload, pos);
        message.half_moves_count = read_u16_be(payload, pos);
        return message;
    }
};
#pragma endregion GameEndMessage

#pragma region AutoMatchRequestMessage 
/*
Send from client to server to request an auto match.

Payload structure:
    - uint8_t username_length (1 byte)
    - char[username_length] username (username_length bytes)
*/
struct AutoMatchRequestMessage
{
    std::string username;

    MessageType getType() const
    {
        return MessageType::AUTO_MATCH_REQUEST;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        return payload;
    }

    static AutoMatchRequestMessage deserialize(const std::vector<uint8_t> &payload)
    {
        AutoMatchRequestMessage message;
        size_t pos = 0;
        message.username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion AutoMatchRequestMessage

#pragma region AutoMatchFoundMessage 
/*
Send from server to clients to notify that an auto match has been found.

Payload structure:
    - uint8_t opponent_username_length (1 byte)
    - char[opponent_username_length] opponent_username (opponent_username_length bytes)
    - uint16_t opponent_elo (2 bytes)
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
*/
struct AutoMatchFoundMessage
{
    std::string opponent_username;
    uint16_t opponent_elo;
    std::string game_id;

    MessageType getType() const
    {
        return MessageType::AUTO_MATCH_FOUND;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(opponent_username.size()));
        payload.insert(payload.end(), opponent_username.begin(), opponent_username.end());

        std::vector<uint8_t> elo_bytes = to_big_endian_16(opponent_elo);
        payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static AutoMatchFoundMessage deserialize(const std::vector<uint8_t> &payload)
    {
        AutoMatchFoundMessage message;
        size_t pos = 0;
        message.opponent_username = read_string(payload, pos);
        message.opponent_elo = read_u16_be(payload, pos);
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion AutoMatchFoundMessage

#pragma region AutoMatchAcceptedMessage =
/*
Send from client to server to accept an auto match.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
*/
struct AutoMatchAcceptedMessage
{
    std::string game_id;

    MessageType getType() const
    {
        return MessageType::AUTO_MATCH_ACCEPTED;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static AutoMatchAcceptedMessage deserialize(const std::vector<uint8_t> &payload)
    {
        AutoMatchAcceptedMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion AutoMatchAcceptedMessage

#pragma region AutoMatchDeclinedMessage =
/*
Send from client to server to decline an auto match.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
*/
struct AutoMatchDeclinedMessage
{
    std::string game_id;

    MessageType getType() const
    {
        return MessageType::AUTO_MATCH_DECLINED;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static AutoMatchDeclinedMessage deserialize(const std::vector<uint8_t> &payload)
    {
        AutoMatchDeclinedMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion AutoMatchDeclinedMessage

#pragma region MatchDeclinedNotificationMessage 
/*
Send from server to client to notify that the opponent has declined the match.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
*/
struct MatchDeclinedNotificationMessage
{
    std::string game_id;

    MessageType getType() const
    {
        return MessageType::MATCH_DECLINED_NOTIFICATION;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static MatchDeclinedNotificationMessage deserialize(const std::vector<uint8_t> &payload)
    {
        MatchDeclinedNotificationMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion MatchDeclinedNotificationMessage

#pragma region RequestPlayerListMessage 
/*
Send from client to server to request the list of players.

Payload structure:
    - No payload
*/
struct RequestPlayerListMessage
{
    MessageType getType() const
    {
        return MessageType::REQUEST_PLAYER_LIST;
    }

    std::vector<uint8_t> serialize() const
    {
        return {}; // No payload
    }

    static RequestPlayerListMessage deserialize(const std::vector<uint8_t> &payload)
    {
        // No payload to deserialize
        return RequestPlayerListMessage();
    }
};
#pragma endregion RequestPlayerListMessage

#pragma region PlayerListMessage 
/*
Send from server to clients to provide the list of players.

Payload structure:
    - uint8_t number_of_players (1 byte)
    - [Player 1][Player 2]...

Player structure:
    - uint8_t username_length (1 byte)
    - char[username_length] username (username_length bytes)
    - uint16_t elo (2 bytes)
    - uint8_t in_game (1 byte)
    - if in_game:
        - uint8_t game_id_length (1 byte)
        - char[game_id_length] game_id (game_id_length bytes)
*/
struct PlayerListMessage
{
    struct Player
    {
        std::string username;
        uint16_t elo;
        bool in_game;
        std::string game_id;
    };

    std::vector<Player> players;

    MessageType getType() const
    {
        return MessageType::PLAYER_LIST;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;
        payload.push_back(static_cast<uint8_t>(players.size()));

        for (const auto &player : players)
        {
            payload.push_back(static_cast<uint8_t>(player.username.size()));
            payload.insert(payload.end(), player.username.begin(), player.username.end());

            std::vector<uint8_t> elo_bytes = to_big_endian_16(player.elo);
            payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());

            payload.push_back(static_cast<uint8_t>(player.in_game));
            
            if (player.in_game) {
                payload.push_back(static_cast<uint8_t>(player.game_id.size()));
                payload.insert(payload.end(), player.game_id.begin(), player.game_id.end());
            }
        }

        return payload;
    }

    static PlayerListMessage deserialize(const std::vector<uint8_t> &payload)
    {
        PlayerListMessage message;
        size_t pos = 0;
        uint8_t number_of_players = read_u8(payload, pos);

        for (uint8_t i = 0; i < number_of_players; ++i)
        {
            Player player;
            player.username = read_string(payload, pos);
            player.elo = read_u16_be(payload, pos);
            player.in_game = static_cast<bool>(read_u8(payload, pos));
            if (player.in_game) {
                player.game_id = read_string(payload, pos);
            }
            message.players.push_back(player);
        }
        return message;
    }
};
#pragma endregion PlayerListMessage

// Chưa xong
#pragma region ChallengeRequestMessage
struct ChallengeRequestMessage
{
    std::string to_username;

    MessageType getType() const
    {
        return MessageType::CHALLENGE_REQUEST;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(to_username.size()));
        payload.insert(payload.end(), to_username.begin(), to_username.end());

        return payload;
    }

    static ChallengeRequestMessage deserialize(const std::vector<uint8_t> &payload)
    {
        ChallengeRequestMessage message;
        size_t pos = 0;
        message.to_username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion ChallengeRequestMessage

#pragma region ChallengeNotificationMessage
struct ChallengeNotificationMessage
{
    std::string from_username;
    uint16_t elo;

    MessageType getType() const
    {
        return MessageType::CHALLENGE_NOTIFICATION;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        std::vector<uint8_t> elo_bytes = to_big_endian_16(elo);
        payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());

        return payload;
    }

    static ChallengeNotificationMessage deserialize(const std::vector<uint8_t> &payload)
    {
        ChallengeNotificationMessage message;
        size_t pos = 0;
        message.from_username = read_string(payload, pos);
        message.elo = read_u16_be(payload, pos);
        return message;
    }
};
#pragma endregion ChallengeNotificationMessage

#pragma region ChallengeResponseMessage
struct ChallengeResponseMessage
{
    enum class Response : uint8_t {
        DECLINED = 0x00,
        ACCEPTED = 0x01
    };
    Response response;

    // caution: this username is challenger's username, not the one challenged
    std::string from_username;

    MessageType getType() const
    {
        return MessageType::CHALLENGE_RESPONSE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Serialize from_username
        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        // Serialize response
        payload.push_back(static_cast<uint8_t>(response));

        return payload;
    }

    static ChallengeResponseMessage deserialize(const std::vector<uint8_t>& payload)
    {
        ChallengeResponseMessage message;
        size_t pos = 0;
        message.from_username = read_string(payload, pos);
        message.response = static_cast<Response>(read_u8(payload, pos));
        return message;
    }
};
#pragma endregion ChallengeResponseMessage

#pragma region ChallengeAcceptedMessage
struct ChallengeAcceptedMessage
{
    std::string from_username;
    std::string game_id;

    MessageType getType() const
    {
        return MessageType::CHALLENGE_ACCEPTED;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Serialize from_username
        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        // Serialize game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static ChallengeAcceptedMessage deserialize(const std::vector<uint8_t>& payload)
    {
        ChallengeAcceptedMessage message;
        size_t pos = 0;
        message.from_username = read_string(payload, pos);
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion ChallengeAcceptedMessage

#pragma region ChallengeDeclinedMessage
struct ChallengeDeclinedMessage
{
    std::string from_username;

    MessageType getType() const
    {
        return MessageType::CHALLENGE_DECLINED;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        return payload;
    }

    static ChallengeDeclinedMessage deserialize(const std::vector<uint8_t> &payload)
    {
        ChallengeDeclinedMessage message;
        size_t pos = 0;
        message.from_username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion ChallengeDeclinedMessage

#pragma region SurrenderMessage
/*
Send from client to server to surrender the game.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
*/
struct SurrenderMessage
{
    std::string game_id;
    std::string from_username;

    MessageType getType() const
    {
        return MessageType::SURRENDER;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        return payload;
    }

    static SurrenderMessage deserialize(const std::vector<uint8_t>& payload)
    {
        SurrenderMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.from_username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion SurrenderMessage

#pragma region ChallengeErrorMessage
/*
Send from server to client to notify that the challenge request is invalid.

Payload structure:
    - uint8_t error_message_length (1 byte)
    - char[error_message_length] error_message (error_message_length bytes)
*/
struct ChallengeErrorMessage
{
    std::string error_message;

    MessageType getType() const
    {
        return MessageType::CHALLENGE_ERROR;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(error_message.size()));
        payload.insert(payload.end(), error_message.begin(), error_message.end());

        return payload;
    }

    static ChallengeErrorMessage deserialize(const std::vector<uint8_t> &payload)
    {
        ChallengeErrorMessage message;

        size_t pos = 0;
        uint8_t error_message_length = payload[pos++];
        message.error_message = std::string(payload.begin() + pos, payload.begin() + pos + error_message_length);

        return message;
    }
};
#pragma endregion ChallengeErrorMessage

#pragma region GameLogMessage
/*
Send from server to both clients after game ends to provide the game log.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
    - uint64_t start_time (8 bytes) - epoch time in nanoseconds
    - uint64_t end_time (8 bytes) - epoch time in nanoseconds
    - uint8_t white_ip_length (1 byte)
    - char[white_ip_length] white_ip (white_ip_length bytes)
    - uint8_t black_ip_length (1 byte)
    - char[black_ip_length] black_ip (black_ip_length bytes)
    - uint8_t winner_length (1 byte)
    - char[winner_length] winner (winner_length bytes)
    - uint8_t reason_length (1 byte)
    - char[reason_length] reason (reason_length bytes)
    - uint16_t moves_count (2 bytes)
    - [Move 1][Move 2]...
    
Move structure:
    - uint8_t uci_move_length (1 byte)
    - char[uci_move_length] uci_move
*/
struct GameLogMessage
{
    std::string game_id;
    int64_t start_time;
    int64_t end_time;
    std::string white_ip;
    std::string black_ip;
    std::string winner;
    std::string reason;
    std::vector<std::string> moves;

    MessageType getType() const
    {
        return MessageType::GAME_LOG;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        // start_time (8 bytes, big endian)
        for (int i = 7; i >= 0; i--)
        {
            payload.push_back(static_cast<uint8_t>((start_time >> (i * 8)) & 0xFF));
        }

        // end_time (8 bytes, big endian)
        for (int i = 7; i >= 0; i--)
        {
            payload.push_back(static_cast<uint8_t>((end_time >> (i * 8)) & 0xFF));
        }

        // white_ip
        payload.push_back(static_cast<uint8_t>(white_ip.size()));
        payload.insert(payload.end(), white_ip.begin(), white_ip.end());

        // black_ip
        payload.push_back(static_cast<uint8_t>(black_ip.size()));
        payload.insert(payload.end(), black_ip.begin(), black_ip.end());

        // winner
        payload.push_back(static_cast<uint8_t>(winner.size()));
        payload.insert(payload.end(), winner.begin(), winner.end());

        // reason
        payload.push_back(static_cast<uint8_t>(reason.size()));
        payload.insert(payload.end(), reason.begin(), reason.end());

        // moves_count (2 bytes, big endian)
        uint16_t moves_count = static_cast<uint16_t>(moves.size());
        payload.push_back(static_cast<uint8_t>((moves_count >> 8) & 0xFF));
        payload.push_back(static_cast<uint8_t>(moves_count & 0xFF));

        // moves
        for (const auto &move : moves)
        {
            payload.push_back(static_cast<uint8_t>(move.size()));
            payload.insert(payload.end(), move.begin(), move.end());
        }

        return payload;
    }

    static GameLogMessage deserialize(const std::vector<uint8_t> &payload)
    {
        GameLogMessage message;

        size_t pos = 0;

        // game_id
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);
        pos += game_id_length;

        // start_time
        message.start_time = 0;
        for (int i = 0; i < 8; i++)
        {
            message.start_time = (message.start_time << 8) | payload[pos++];
        }

        // end_time
        message.end_time = 0;
        for (int i = 0; i < 8; i++)
        {
            message.end_time = (message.end_time << 8) | payload[pos++];
        }

        // white_ip
        uint8_t white_ip_length = payload[pos++];
        message.white_ip = std::string(payload.begin() + pos, payload.begin() + pos + white_ip_length);
        pos += white_ip_length;

        // black_ip
        uint8_t black_ip_length = payload[pos++];
        message.black_ip = std::string(payload.begin() + pos, payload.begin() + pos + black_ip_length);
        pos += black_ip_length;

        // winner
        uint8_t winner_length = payload[pos++];
        message.winner = std::string(payload.begin() + pos, payload.begin() + pos + winner_length);
        pos += winner_length;

        // reason
        uint8_t reason_length = payload[pos++];
        message.reason = std::string(payload.begin() + pos, payload.begin() + pos + reason_length);
        pos += reason_length;

        // moves_count
        uint16_t moves_count = (static_cast<uint16_t>(payload[pos]) << 8) | static_cast<uint16_t>(payload[pos + 1]);
        pos += 2;

        // moves
        for (uint16_t i = 0; i < moves_count; i++)
        {
            uint8_t move_length = payload[pos++];
            message.moves.push_back(std::string(payload.begin() + pos, payload.begin() + pos + move_length));
            pos += move_length;
        }

        return message;
    }
};
#pragma endregion GameLogMessage

#endif // MESSAGE_HPP
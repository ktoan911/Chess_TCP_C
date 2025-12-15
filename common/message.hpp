#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>
#include <memory>

#include "utils.hpp"
#include "protocol.hpp"

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
        uint8_t username_length = payload[pos++];
        message.username = std::string(payload.begin() + pos, payload.begin() + pos + username_length);

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
        uint8_t username_length = payload[pos++];
        message.username = std::string(payload.begin() + pos, payload.begin() + pos + username_length);

        pos += username_length;
        message.elo = from_big_endian_16(payload, pos);

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
        uint8_t error_message_length = payload[pos++];
        message.error_message = std::string(payload.begin() + pos, payload.begin() + pos + error_message_length);

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
        uint8_t username_length = payload[pos++];
        message.username = std::string(payload.begin() + pos, payload.begin() + pos + username_length);

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
        uint8_t username_length = payload[pos++];
        message.username = std::string(payload.begin() + pos, payload.begin() + pos + username_length);

        pos += username_length;
        message.elo = from_big_endian_16(payload, pos);

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
        uint8_t error_message_length = payload[pos++];
        message.error_message = std::string(payload.begin() + pos, payload.begin() + pos + error_message_length);

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
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

        pos += game_id_length;
        uint8_t player1_username_length = payload[pos++];
        message.player1_username = std::string(payload.begin() + pos, payload.begin() + pos + player1_username_length);

        pos += player1_username_length;
        uint8_t player2_username_length = payload[pos++];
        message.player2_username = std::string(payload.begin() + pos, payload.begin() + pos + player2_username_length);

        pos += player2_username_length;
        uint8_t starting_player_username_length = payload[pos++];
        message.starting_player_username = std::string(payload.begin() + pos, payload.begin() + pos + starting_player_username_length);

        pos += starting_player_username_length;
        uint8_t fen_length = payload[pos++];
        message.fen = std::string(payload.begin() + pos, payload.begin() + pos + fen_length);

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
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

        pos += game_id_length;
        uint8_t uci_move_length = payload[pos++];
        message.uci_move = std::string(payload.begin() + pos, payload.begin() + pos + uci_move_length);

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
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

        pos += game_id_length;
        uint8_t error_message_length = payload[pos++];
        message.error_message = std::string(payload.begin() + pos, payload.begin() + pos + error_message_length);

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
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

        pos += game_id_length;
        uint8_t fen_length = payload[pos++];
        message.fen = std::string(payload.begin() + pos, payload.begin() + pos + fen_length);

        pos += fen_length;
        uint8_t current_turn_username_length = payload[pos++];
        message.current_turn_username = std::string(payload.begin() + pos, payload.begin() + pos + current_turn_username_length);

        pos += current_turn_username_length;
        message.is_game_over = payload[pos++];

        uint8_t message_length = payload[pos++];
        message.message = std::string(payload.begin() + pos, payload.begin() + pos + message_length);

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
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

        pos += game_id_length;
        uint8_t winner_username_length = payload[pos++];
        message.winner_username = std::string(payload.begin() + pos, payload.begin() + pos + winner_username_length);
        
        pos += winner_username_length;
        uint8_t reason_length = payload[pos++];
        message.reason = std::string(payload.begin() + pos, payload.begin() + pos + reason_length);

        pos += reason_length;
        message.half_moves_count = from_big_endian_16(payload, pos);
        
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
        uint8_t username_length = payload[pos++];
        message.username = std::string(payload.begin() + pos, payload.begin() + pos + username_length);

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
        uint8_t opponent_username_length = payload[pos++];
        message.opponent_username = std::string(payload.begin() + pos, payload.begin() + pos + opponent_username_length);

        pos += opponent_username_length;
        message.opponent_elo = from_big_endian_16(payload, pos);

        pos += 2;
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

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
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

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
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

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
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

        return message;
    }
};
#pragma endregion MatchDeclinedNotificationMessage

#pragma region PlayWithBotMessage
/*
Send from client to server to play with bot.

Payload structure:
    - uint8_t username_length (1 byte)
    - char[username_length] username (username_length bytes)
*/

struct PlayWithBotMessage
{
    std::string username;

    MessageType getType() const
    {
        return MessageType::PLAY_WITH_BOT;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        return payload;
    }

    static PlayWithBotMessage deserialize(const std::vector<uint8_t> &payload)
    {
        PlayWithBotMessage message;

        size_t pos = 0;
        uint8_t username_length = payload[pos++];
        message.username = std::string(payload.begin() + pos, payload.begin() + pos + username_length);

        return message;
    }
};

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
        uint8_t number_of_players = payload[pos++];

        for (uint8_t i = 0; i < number_of_players; ++i)
        {
            Player player;
            
            uint8_t username_length = payload[pos++];
            player.username = std::string(payload.begin() + pos, payload.begin() + pos + username_length);
            pos += username_length;

            player.elo = from_big_endian_16(payload, pos);
            pos += 2;

            player.in_game = payload[pos++];
            
            if (player.in_game) {
                uint8_t game_id_length = payload[pos++];
                player.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);
                pos += game_id_length;
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
        uint8_t to_username_length = payload[pos++];
        message.to_username = std::string(payload.begin() + pos, payload.begin() + pos + to_username_length);

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
        uint8_t from_username_length = payload[pos++];
        message.from_username = std::string(payload.begin() + pos, payload.begin() + pos + from_username_length);

        pos += from_username_length;
        message.elo = from_big_endian_16(payload, pos);

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

        // Deserialize from_username
        uint8_t from_username_length = payload[pos++];
        message.from_username = std::string(payload.begin() + pos, payload.begin() + pos + from_username_length);
        pos += from_username_length;

        // Deserialize response
        message.response = static_cast<Response>(payload[pos++]);

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

        // Deserialize from_username
        uint8_t from_username_length = payload[pos++];
        message.from_username = std::string(payload.begin() + pos, payload.begin() + pos + from_username_length);
        pos += from_username_length;

        // Deserialize game_id
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

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
        uint8_t from_username_length = payload[pos++];
        message.from_username = std::string(payload.begin() + pos, payload.begin() + pos + from_username_length);

        return message;
    }
};
#pragma endregion ChallengeDeclinedMessage

#pragma region RequestSpectateMessage
/*
Send from client to server to request spectating a player's match.

Payload structure:
    - uint8_t username_length (1 byte)
    - char[username_length] username (username_length bytes)
*/
struct RequestSpectateMessage
{
    std::string username;

    MessageType getType() const
    {
        return MessageType::REQUEST_SPECTATE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        return payload;
    }

    static RequestSpectateMessage deserialize(const std::vector<uint8_t> &payload)
    {
        RequestSpectateMessage message;

        size_t pos = 0;
        uint8_t username_length = payload[pos++];
        message.username = std::string(payload.begin() + pos, payload.begin() + pos + username_length);

        return message;
    }
};
#pragma endregion RequestSpectateMessage

#pragma region SpectateSuccessMessage
struct SpectateSuccessMessage
{
    std::string game_id;

    MessageType getType() const
    {
        return MessageType::SPECTATE_SUCCESS;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static SpectateSuccessMessage deserialize(const std::vector<uint8_t> &payload)
    {
        SpectateSuccessMessage message;
        size_t pos = 0;

        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

        return message;
    }
};
#pragma endregion SpectateSuccessMessage

#pragma region SpectateFailureMessage
struct SpectateFailureMessage
{
    MessageType getType() const
    {
        return MessageType::SPECTATE_FAILURE;
    }

    std::vector<uint8_t> serialize() const
    {
        return {}; // No payload
    }

    static SpectateFailureMessage deserialize(const std::vector<uint8_t> &payload)
    {
        return SpectateFailureMessage(); // No payload to deserialize
    }
};
#pragma endregion SpectateFailureMessage

#pragma region SpectateMoveMessage
/*
Send from server to spectator client to update the game state.

Payload structure:
    - uint8_t fen_length (1 byte)
    - char[fen_length] fen (fen_length bytes)
    - uint8_t current_turn_username_length (1 byte)
    - char[current_turn_username_length] current_turn_username (current_turn_username_length bytes)
    - uint8_t is_white (1 byte)
*/
struct SpectateMoveMessage
{
    std::string fen;
    std::string current_turn_username;
    bool is_white;

    MessageType getType() const
    {
        return MessageType::SPECTATE_MOVE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(fen.size()));
        payload.insert(payload.end(), fen.begin(), fen.end());

        payload.push_back(static_cast<uint8_t>(current_turn_username.size()));
        payload.insert(payload.end(), current_turn_username.begin(), current_turn_username.end());

        payload.push_back(static_cast<uint8_t>(is_white));

        return payload;
    }

    static SpectateMoveMessage deserialize(const std::vector<uint8_t> &payload)
    {
        SpectateMoveMessage message;
        size_t pos = 0;

        uint8_t fen_length = payload[pos++];
        message.fen = std::string(payload.begin() + pos, payload.begin() + pos + fen_length);
        pos += fen_length;

        uint8_t username_length = payload[pos++];
        message.current_turn_username = std::string(payload.begin() + pos, payload.begin() + pos + username_length);
        pos += username_length;

        message.is_white = payload[pos] != 0;

        return message;
    }
};
#pragma endregion SpectateMoveMessage

#pragma region SpectateEndMessage
struct SpectateEndMessage
{
    MessageType getType() const
    {
        return MessageType::SPECTATE_END;
    }

    std::vector<uint8_t> serialize() const
    {
        return {}; // No payload
    }

    static SpectateEndMessage deserialize(const std::vector<uint8_t> &payload)
    {
        return SpectateEndMessage(); // No payload to deserialize
    }
};
#pragma endregion SpectateEndMessage

#pragma region SpectateExitMessage
/*
Send from client to server to exit spectating a game.

Payload structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
*/
struct SpectateExitMessage
{
    std::string game_id;

    MessageType getType() const
    {
        return MessageType::SPECTATE_EXIT;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static SpectateExitMessage deserialize(const std::vector<uint8_t> &payload)
    {
        SpectateExitMessage message;

        size_t pos = 0;
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);

        return message;
    }
};
#pragma endregion SpectateExitMessage
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
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);
        pos += game_id_length;

        uint8_t from_username_length = payload[pos++];
        message.from_username = std::string(payload.begin() + pos, payload.begin() + pos + from_username_length);

        return message;
    }
};
#pragma endregion SurrenderMessage

#pragma region RequestMatchHistoryMessage
/*
Send from client to server to request the match history of the player.

Payload structure:
    - No payload
*/
struct RequestMatchHistoryMessage {
    MessageType getType() const {
        return MessageType::REQUEST_MATCH_HISTORY;
    }

    std::vector<uint8_t> serialize() const {
        // No payload
        return {};
    }

    static RequestMatchHistoryMessage deserialize(const std::vector<uint8_t>& payload) {
        // No payload to deserialize
        return RequestMatchHistoryMessage();
    }
};
#pragma endregion RequestMatchHistoryMessage

#pragma region MatchHistoryMessage
/*
Send from server to client to provide the match history of a player.

Payload structure:
    - uint8_t number_of_matches (1 byte)
    - [Match 1][Match 2]...

Match structure:
    - uint8_t game_id_length (1 byte)
    - char[game_id_length] game_id (game_id_length bytes)
    - uint8_t opponent_username_length (1 byte)
    - char[opponent_username_length] opponent_username (opponent_username_length bytes)
    - uint8_t won (1 byte)
    - uint8_t date_length (1 byte)
    - char[date_length] date (date_length bytes)
*/
struct  MatchHistoryMessage {
    struct Match {
        std::string game_id;
        std::string opponent_username;
        bool won;
        std::string date;
    };

    std::vector<Match> matches;

    MessageType getType() const {
        return MessageType::MATCH_HISTORY;
    }

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> payload;
        payload.push_back(static_cast<uint8_t>(matches.size()));

        for (const auto& match : matches) {
            payload.push_back(static_cast<uint8_t>(match.game_id.size()));
            payload.insert(payload.end(), match.game_id.begin(), match.game_id.end());

            payload.push_back(static_cast<uint8_t>(match.opponent_username.size()));
            payload.insert(payload.end(), match.opponent_username.begin(), match.opponent_username.end());

            payload.push_back(static_cast<uint8_t>(match.won));

            payload.push_back(static_cast<uint8_t>(match.date.size()));
            payload.insert(payload.end(), match.date.begin(), match.date.end());
        }

        return payload;
    }

    static MatchHistoryMessage deserialize(const std::vector<uint8_t>& payload) {
        MatchHistoryMessage message;

        size_t pos = 0;
        uint8_t number_of_matches = payload[pos++];

        for (uint8_t i = 0; i < number_of_matches; ++i) {
            Match match;

            uint8_t game_id_length = payload[pos++];
            match.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);
            pos += game_id_length;

            uint8_t opponent_username_length = payload[pos++];
            match.opponent_username = std::string(payload.begin() + pos, payload.begin() + pos + opponent_username_length);
            pos += opponent_username_length;

            match.won = payload[pos++];

            uint8_t date_length = payload[pos++];
            match.date = std::string(payload.begin() + pos, payload.begin() + pos + date_length);
            pos += date_length;

            message.matches.push_back(match);
        }

        return message;
    }
};
#pragma endregion MatchHistoryMessage

#endif // MESSAGE_HPP
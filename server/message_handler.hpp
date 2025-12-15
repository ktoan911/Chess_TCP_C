#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "../common/protocol.hpp"
#include "../common/message.hpp"
#include "../common/const.hpp"
#include "../common/utils.hpp"

#include "data_storage.hpp"
#include "network_server.hpp"
#include "game_manager.hpp"

class MessageHandler
{
public:
    // Handle incoming messages ================================================================================

    void handleMessage(int client_fd, const Packet &packet)
    {
        switch (packet.type)
        {
        case MessageType::REGISTER:
            // Handle register
            handleRegister(client_fd, packet.payload);
            break;
        case MessageType::LOGIN:
            // Handle login
            handleLogin(client_fd, packet.payload);
            break;
        case MessageType::MOVE:
            // Handle move
            handleMove(client_fd, packet.payload);
            break;

        case MessageType::AUTO_MATCH_REQUEST:
            // Handle auto match request
            handleAutoMatchRequest(client_fd, packet.payload);
            break;
        case MessageType::AUTO_MATCH_ACCEPTED:
            // Handle auto match accepted
            handleAutoMatchAccepted(client_fd, packet.payload);
            break;
        case MessageType::AUTO_MATCH_DECLINED:
            // Handle auto match declined
            handleAutoMatchDeclined(client_fd, packet.payload);
            break;
        
        case MessageType::PLAY_WITH_BOT:
            // Handle play with bot
            handlePlayWithBot(client_fd, packet.payload);
            break;

            // Add additional cases for other MessageTypes

        case MessageType::REQUEST_PLAYER_LIST:
            // Handle request player list
            handleRequestPlayerList(client_fd, packet.payload);
            break;

        case MessageType::CHALLENGE_REQUEST:
            // Handle incoming challenge request
            handleChallengeRequest(client_fd, packet.payload);
            break;

        case MessageType::CHALLENGE_RESPONSE:
            // Handle incoming challenge response
            handleChallengeResponse(client_fd, packet.payload);
            break;

        case MessageType::REQUEST_SPECTATE:
            // Handle request to spectate a game
            handleRequestSpectate(client_fd, packet.payload);
            break;

        case MessageType::SPECTATE_EXIT:
            // Handle spectator exit
            handleSpectateExit(client_fd, packet.payload);
            break;

        case MessageType::SURRENDER:
            handleSurrender(client_fd, packet.payload);
            break;

        case MessageType::REQUEST_MATCH_HISTORY:
            // Handle request match history
            handleRequestMatchHistory(client_fd, packet.payload);
            break;

        default:
            // Handle unknown message type
            handleUnknown(client_fd, packet.payload);
            break;
        }
    }

private:
    // Handle specific message types ============================================================================

    void handleUnknown(int client_fd, const std::vector<uint8_t> &payload)
    {
        std::cout << "[UNKNOWN]" << std::endl;
    }

    void handleRegister(int client_fd, const std::vector<uint8_t> &payload)
    {
        RegisterMessage message = RegisterMessage::deserialize(payload);

        std::cout << "[REGISTER] username: " << message.username << std::endl;

        DataStorage &storage = DataStorage::getInstance();
        bool isUserValid = storage.registerUser(message.username);

        NetworkServer &server = NetworkServer::getInstance();

        if (isUserValid)
        {
            RegisterSuccessMessage successMessage;
            successMessage.username = message.username;
            successMessage.elo = Const::DEFAULT_ELO;
            std::vector<uint8_t> serialized = successMessage.serialize();
            server.sendPacket(client_fd, successMessage.getType(), serialized);
            server.setUsername(client_fd, message.username);
        }
        else
        {
            RegisterFailureMessage failureMessage;
            failureMessage.error_message = "Username already exists.";
            std::vector<uint8_t> serialized = failureMessage.serialize();
            server.sendPacket(client_fd, failureMessage.getType(), serialized);
        }
    }

    void handleLogin(int client_fd, const std::vector<uint8_t> &payload)
    {
        LoginMessage message = LoginMessage::deserialize(payload);

        std::cout << "[LOGIN] username: " << message.username << ", client_fd: " << client_fd << std::endl;

        DataStorage &storage = DataStorage::getInstance();
        bool isUserValid = storage.validateUser(message.username);

        NetworkServer &server = NetworkServer::getInstance();

        bool isLoggedIn = server.isUserLoggedIn(message.username);

        if (isUserValid && !isLoggedIn)
        {
            uint16_t elo = storage.getUserELO(message.username);

            LoginSuccessMessage successMessage;
            successMessage.username = message.username;
            successMessage.elo = elo;
            std::vector<uint8_t> serialized = successMessage.serialize();
            server.sendPacket(client_fd, successMessage.getType(), serialized);
            server.setUsername(client_fd, message.username);
        }
        else if (!isUserValid)
        {
            LoginFailureMessage failureMessage;
            failureMessage.error_message = "Invalid username.";
            std::vector<uint8_t> serialized = failureMessage.serialize();
            server.sendPacket(client_fd, failureMessage.getType(), serialized);
        }
        else
        {
            LoginFailureMessage failureMessage;
            failureMessage.error_message = "User already logged in.";
            std::vector<uint8_t> serialized = failureMessage.serialize();
            server.sendPacket(client_fd, failureMessage.getType(), serialized);
        }
    }

    void handleMove(int client_fd, const std::vector<uint8_t> &payload)
    {
        MoveMessage message = MoveMessage::deserialize(payload);

        std::cout << "[MOVE] game_id: " << message.game_id
                  << ", uci_move: " << message.uci_move << std::endl;

        GameManager::getInstance().handleMove(client_fd, message.game_id, message.uci_move);
    }

    void handleAutoMatchRequest(int client_fd, const std::vector<uint8_t> &payload)
    {
        AutoMatchRequestMessage message = AutoMatchRequestMessage::deserialize(payload);

        std::cout << "[AUTO_MATCH_REQUEST] username: " << message.username << std::endl;

        NetworkServer::getInstance().setUsername(client_fd, message.username);

        GameManager::getInstance().addPlayerToQueue(client_fd);
    }

    void handleAutoMatchAccepted(int client_fd, const std::vector<uint8_t> &payload)
    {
        AutoMatchAcceptedMessage message = AutoMatchAcceptedMessage::deserialize(payload);

        std::cout << "[AUTO_MATCH_ACCEPTED] game_id: " << message.game_id << std::endl;

        GameManager::getInstance().handleAutoMatchAccepted(client_fd, message.game_id);
    }

    void handleAutoMatchDeclined(int client_fd, const std::vector<uint8_t> &payload)
    {
        AutoMatchDeclinedMessage message = AutoMatchDeclinedMessage::deserialize(payload);

        std::cout << "[AUTO_MATCH_DECLINED] game_id: " << message.game_id << std::endl;

        GameManager::getInstance().handleAutoMatchDeclined(client_fd, message.game_id);
    }

    void handleRequestPlayerList(int client_fd, const std::vector<uint8_t> &payload)
    {
        DataStorage &storage = DataStorage::getInstance();
        NetworkServer &server = NetworkServer::getInstance();
        GameManager &gameManager = GameManager::getInstance();

        RequestPlayerListMessage message = RequestPlayerListMessage::deserialize(payload);

        std::cout << "[REQUEST_PLAYER_LIST] from " << server.getUsername(client_fd) << std::endl;

        std::unordered_map<std::string, UserModel> players = storage.getPlayerList();
        PlayerListMessage response;

        for (const auto &pair : players)
        {
            PlayerListMessage::Player player;
            player.username = pair.second.username;
            player.elo = pair.second.elo;
            player.in_game = gameManager.isUserInGame(player.username);

            if (player.in_game)
            {
                player.game_id = gameManager.getUserGameId(player.username);
            }

            if (server.isUserLoggedIn(player.username))
            {
                response.players.push_back(player);
            }
        }

        server.sendPacket(client_fd, response.getType(), response.serialize());
    }

    void handleChallengeRequest(int client_fd, const std::vector<uint8_t> &payload)
    {
        ChallengeRequestMessage message = ChallengeRequestMessage::deserialize(payload);
        NetworkServer &server = NetworkServer::getInstance();
        DataStorage &storage = DataStorage::getInstance();

        std::cout << "[CHALLENGE_REQUEST] from: " << server.getUsername(client_fd)
                  << ", to: " << message.to_username << std::endl;

        bool is_opponent_online = server.isUserLoggedIn(message.to_username);
        std::cout << "Opponent " << message.to_username << " online: " << is_opponent_online << std::endl;

        if (is_opponent_online)
        {
            int to_client_fd = server.getClientFD(message.to_username);

            // to complete
            ChallengeNotificationMessage notification_msg;
            notification_msg.from_username = server.getUsername(client_fd);
            notification_msg.elo = storage.getUserELO(notification_msg.from_username);
            std::vector<uint8_t> serialized = notification_msg.serialize();
            server.sendPacket(to_client_fd, notification_msg.getType(), serialized);

            std::cout << "[CHALLENGE_NOTIFICATION] Sent challenge from "
                      << server.getUsername(client_fd)
                      << " to " << message.to_username << std::endl;
        }
        else
        {
            // to complete
        }
    }

    void handleChallengeResponse(int client_fd, const std::vector<uint8_t> &payload)
    {
        ChallengeResponseMessage message = ChallengeResponseMessage::deserialize(payload);
        NetworkServer &network_server = NetworkServer::getInstance();
        GameManager &gameManager = GameManager::getInstance();

        std::string challenger_username = message.from_username;
        std::string challenged_username = network_server.getUsername(client_fd);

        int challenger_fd = network_server.getClientFD(challenger_username);
        // challenged_fd is client_fd

        std::cout << "[CHALLENGE_RESPONSE] from: " << challenged_username
                  << ", challenged by: " << challenger_username
                  << ", accepted: " << static_cast<uint8_t>(message.response) << std::endl;

        if (message.response == ChallengeResponseMessage::Response::ACCEPTED)
        {
            std::string game_id = gameManager.createGame(challenger_username, challenged_username);

            // Is handling PendingGame required? Review game_manager

            ChallengeAcceptedMessage challenge_accepted_msg;
            challenge_accepted_msg.from_username = challenged_username;
            challenge_accepted_msg.game_id = game_id;

            std::vector<uint8_t> serialized = challenge_accepted_msg.serialize();
            network_server.sendPacket(challenger_fd, challenge_accepted_msg.getType(), serialized);

            std::cout << "Game " << game_id << " started." << std::endl;

            // Notify both players about the game start
            GameStartMessage game_start_msg;
            game_start_msg.game_id = game_id;
            game_start_msg.player1_username = challenger_username;
            game_start_msg.player2_username = challenged_username;

            game_start_msg.starting_player_username = game_start_msg.player1_username; // Player 1 starts
            game_start_msg.fen = chess::constants::STARTPOS;

            std::vector<uint8_t> gs_serialized = game_start_msg.serialize();

            network_server.sendPacket(challenger_fd, MessageType::GAME_START, gs_serialized);
            network_server.sendPacket(client_fd, MessageType::GAME_START, gs_serialized);
        }
        else
        {
            // If the challenge was declined, send a message to the challenger
            ChallengeDeclinedMessage challenge_declined_msg;
            challenge_declined_msg.from_username = network_server.getUsername(client_fd);
            std::vector<uint8_t> serialized = challenge_declined_msg.serialize();
            network_server.sendPacket(challenger_fd, challenge_declined_msg.getType(), serialized);

            std::cout << "Decline message sent to " << message.from_username << std::endl;
        }
    }

    void handlePlayWithBot(int client_fd, const std::vector<uint8_t> &payload)
    {
        PlayWithBotMessage message = PlayWithBotMessage::deserialize(payload);
        NetworkServer &network_server = NetworkServer::getInstance();
        GameManager &gameManager = GameManager::getInstance();

        std::string username = network_server.getUsername(client_fd);

        std::cout << "[PLAY_WITH_BOT] from: " << username << std::endl;

        std::string game_id = gameManager.createGameWithBot(username);

        // Notify the player about the game start
        GameStartMessage game_start_msg;
        game_start_msg.game_id = game_id;
        game_start_msg.player1_username = username;
        game_start_msg.player2_username = "Bot";
        game_start_msg.starting_player_username = username; // Player 1 starts
        game_start_msg.fen = chess::constants::STARTPOS;

        std::vector<uint8_t> serialized = game_start_msg.serialize();
        network_server.sendPacket(client_fd, MessageType::GAME_START, serialized);

        std::cout << "Game " << game_id << " started." << std::endl;
    }

    void handleRequestSpectate(int client_fd, const std::vector<uint8_t> &payload)
    {
        RequestSpectateMessage message = RequestSpectateMessage::deserialize(payload);
        NetworkServer &network_server = NetworkServer::getInstance();
        GameManager &gameManager = GameManager::getInstance();

        std::string playing_username = message.username;
        std::string requester_username = network_server.getUsername(client_fd);

        bool is_playing = gameManager.isUserInGame(playing_username);

        std::cout << "[REQUEST_SPECTATE] from: " << requester_username << ", to watch: " << playing_username << std::endl;

        if (is_playing)
        {
            std::string game_id = gameManager.getUserGameId(playing_username);

            gameManager.addSpectator(game_id, client_fd);

            SpectateSuccessMessage spectate_success_msg;
            spectate_success_msg.game_id = game_id;
            network_server.sendPacket(client_fd, spectate_success_msg.getType(), spectate_success_msg.serialize());

            std::cout << "Spectate success message sent to " << requester_username << std::endl;
        }
        else
        {
            // Notify the requester that the player is not in a game
            SpectateFailureMessage spectate_failure_msg;
            network_server.sendPacket(client_fd, spectate_failure_msg.getType(), spectate_failure_msg.serialize());

            std::cout << "Spectate failure message sent to " << requester_username << std::endl;
        }
    }

    void handleSpectateExit(int client_fd, const std::vector<uint8_t> &payload)
    {
        SpectateExitMessage message = SpectateExitMessage::deserialize(payload);
        NetworkServer &network_server = NetworkServer::getInstance();
        GameManager &gameManager = GameManager::getInstance();

        std::string game_id = message.game_id;
        std::string username = network_server.getUsername(client_fd);

        gameManager.removeSpectator(game_id, client_fd);

        std::cout << "[SPECTATE_EXIT] " << username << " exited spectating game " << game_id << std::endl;
    }
    
    void handleSurrender(int client_fd, const std::vector<uint8_t> &payload)
    {
        SurrenderMessage message = SurrenderMessage::deserialize(payload);

        std::cout << "[SURRENDER] game_id: " << message.game_id
                  << ", from_username: " << message.from_username << std::endl;

        NetworkServer &server = NetworkServer::getInstance();
        GameManager &game_manager = GameManager::getInstance();

        std::string surrendering_player = server.getUsername(client_fd);
        std::string opponent_username = game_manager.getOpponent(message.game_id, surrendering_player);

        if (opponent_username.empty())
        {
            std::cerr << "Error: Could not find opponent for game_id: " << message.game_id << std::endl;
            return;
        }

        // Dừng trận đấu
        game_manager.endGameForSurrender(message.game_id, message.from_username);

        // Thông báo kết thúc trò chơi
        GameEndMessage end_message;
        end_message.game_id = message.game_id;
        end_message.winner_username = opponent_username;
        end_message.reason = surrendering_player + " has surrendered.";
        end_message.half_moves_count = game_manager.getGameHalfMovesCount(message.game_id);

        std::vector<uint8_t> serialized = end_message.serialize();
        server.sendPacket(client_fd, end_message.getType(), serialized); // Người đầu hàng
        int opponent_fd = server.getClientFD(opponent_username);
        server.sendPacket(opponent_fd, end_message.getType(), serialized); // Đối thủ
    }

    void handleRequestMatchHistory(int client_fd, const std::vector<uint8_t> &payload)
    {
        RequestMatchHistoryMessage message = RequestMatchHistoryMessage::deserialize(payload);
        NetworkServer &server = NetworkServer::getInstance();
        DataStorage &storage = DataStorage::getInstance();

        std::string username = server.getUsername(client_fd);

        std::cout << "[REQUEST_MATCH_HISTORY] from " << username << std::endl;

        std::vector<MatchModel> matches = storage.getMatchHistory(username);

        // Cast the matches to MatchHistoryMessage
        MatchHistoryMessage response;
        for (const MatchModel &match : matches)
        {
            MatchHistoryMessage::Match match_history;
            match_history.game_id = match.game_id;
            match_history.opponent_username = username == match.white_username ? match.black_username : match.white_username;
            match_history.won = match.result == username;
            match_history.date = match.start_time.time_since_epoch().count();

            response.matches.push_back(match_history);
        }

        server.sendPacket(client_fd, response.getType(), response.serialize());
    }
};

#endif // MESSAGE_HANDLER_HPP
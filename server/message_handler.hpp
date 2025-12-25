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



        case MessageType::SURRENDER:
            handleSurrender(client_fd, packet.payload);
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

        std::string from_username = server.getUsername(client_fd);
        std::string to_username = message.to_username;

        std::cout << "[CHALLENGE_REQUEST] from: " << from_username
                  << ", to: " << to_username << std::endl;

        // Check if opponent is online
        bool is_opponent_online = server.isUserLoggedIn(to_username);
        std::cout << "Opponent " << to_username << " online: " << is_opponent_online << std::endl;

        if (!is_opponent_online)
        {
            // Opponent not online, send error
            ChallengeErrorMessage error_msg;
            error_msg.error_message = "Player " + to_username + " is not online.";
            std::vector<uint8_t> serialized = error_msg.serialize();
            server.sendPacket(client_fd, error_msg.getType(), serialized);
            return;
        }

        // Check rank difference - only allow challenging players within ±10 ranks
        int from_rank = storage.getUserRank(from_username);
        int to_rank = storage.getUserRank(to_username);
        int rank_difference = abs(from_rank - to_rank);
        
        std::cout << "Rank check - From: " << from_username << " (rank " << from_rank 
                  << "), To: " << to_username << " (rank " << to_rank 
                  << "), Difference: " << rank_difference << std::endl;

        if (rank_difference > 10)
        {
            // Rank difference too large, send error
            ChallengeErrorMessage error_msg;
            error_msg.error_message = "Cannot challenge " + to_username + ". Rank difference is " 
                                      + std::to_string(rank_difference) + " (max allowed: 10).";
            std::vector<uint8_t> serialized = error_msg.serialize();
            server.sendPacket(client_fd, error_msg.getType(), serialized);
            
            std::cout << "[CHALLENGE_ERROR] Rank difference too large: " << rank_difference << std::endl;
            return;
        }

        // All checks passed, send challenge notification to opponent
        int to_client_fd = server.getClientFD(to_username);

        ChallengeNotificationMessage notification_msg;
        notification_msg.from_username = from_username;
        notification_msg.elo = storage.getUserELO(from_username);
        std::vector<uint8_t> serialized = notification_msg.serialize();
        server.sendPacket(to_client_fd, notification_msg.getType(), serialized);

        std::cout << "[CHALLENGE_NOTIFICATION] Sent challenge from "
                  << from_username << " to " << to_username << std::endl;
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
};

#endif // MESSAGE_HANDLER_HPP
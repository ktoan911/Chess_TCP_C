#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <string>
#include <vector>
#include <iostream>

#include "../common/protocol.hpp"
#include "../common/message.hpp"

#include "client_state.hpp"
#include "session_data.hpp"
#include "ui.hpp"

/**
 * @brief Xử lý message từ server một cách đồng bộ
 * 
 * Phiên bản single-threaded: Không tạo thread, xử lý trực tiếp
 * và trả về state mới cho event loop.
 */
class MessageHandler
{
public:
    /**
     * @brief Xử lý message từ server và trả về state mới
     * 
     * @param currentState State hiện tại
     * @param packet Message từ server
     * @param context State context để lưu data tạm
     * @return ClientState mới sau khi xử lý
     */
    ClientState handleMessage(ClientState currentState, const Packet &packet, StateContext &context)
    {
        switch (packet.type)
        {
        case MessageType::REGISTER_SUCCESS:
            return handleRegisterSuccess(packet.payload);
            
        case MessageType::REGISTER_FAILURE:
            return handleRegisterFailure(packet.payload);
            
        case MessageType::LOGIN_SUCCESS:
            return handleLoginSuccess(packet.payload);
            
        case MessageType::LOGIN_FAILURE:
            return handleLoginFailure(packet.payload);

        case MessageType::GAME_START:
            return handleGameStart(packet.payload);
            
        case MessageType::GAME_STATUS_UPDATE:
            return handleGameStatusUpdate(packet.payload);
            
        case MessageType::INVALID_MOVE:
            return handleInvalidMove(packet.payload);
            
        case MessageType::GAME_END:
            return handleGameEnd(packet.payload);

        case MessageType::CHALLENGE_NOTIFICATION:
            return handleChallengeNotification(packet.payload, context);

        case MessageType::AUTO_MATCH_FOUND:
            return handleAutoMatchFound(packet.payload, context);
            
        case MessageType::MATCH_DECLINED_NOTIFICATION:
            return handleMatchDeclinedNotification(packet.payload);

        case MessageType::PLAYER_LIST:
            return handlePlayerList(packet.payload, context);
            
        case MessageType::CHALLENGE_DECLINED:
            return handleChallengeDeclined(packet.payload);
            
        case MessageType::CHALLENGE_ACCEPTED:
            return handleChallengeAccepted(packet.payload);
            
        case MessageType::CHALLENGE_ERROR:
            return handleChallengeError(packet.payload);

        default:
            std::cout << "[WARNING] Unknown message type: " 
                      << static_cast<int>(packet.type) << std::endl;
            return currentState;
        }
    }

private:
    // ==================== Auth handlers ====================
    
    ClientState handleRegisterSuccess(const std::vector<uint8_t> &payload)
    {
        RegisterSuccessMessage message = RegisterSuccessMessage::deserialize(payload);
        
        UI::clearConsole();
        UI::printSuccessMessage("Đăng ký thành công!");
        std::cout << "Username: " << message.username << std::endl;
        std::cout << "ELO: " << message.elo << std::endl;

        SessionData &session = SessionData::getInstance();
        session.setUsername(message.username);
        session.setElo(message.elo);

        UI::displayGameMenuPrompt();
        return ClientState::GAME_MENU;
    }

    ClientState handleRegisterFailure(const std::vector<uint8_t> &payload)
    {
        RegisterFailureMessage message = RegisterFailureMessage::deserialize(payload);
        
        UI::printErrorMessage("Đăng ký thất bại: " + message.error_message);
        UI::displayInitialMenuPrompt();
        return ClientState::INITIAL_MENU;
    }

    ClientState handleLoginSuccess(const std::vector<uint8_t> &payload)
    {
        LoginSuccessMessage message = LoginSuccessMessage::deserialize(payload);
        
        UI::clearConsole();
        UI::printSuccessMessage("Đăng nhập thành công!");
        std::cout << "Username: " << message.username << std::endl;
        std::cout << "ELO: " << message.elo << std::endl;

        SessionData &session = SessionData::getInstance();
        session.setUsername(message.username);
        session.setElo(message.elo);

        UI::displayGameMenuPrompt();
        return ClientState::GAME_MENU;
    }

    ClientState handleLoginFailure(const std::vector<uint8_t> &payload)
    {
        LoginFailureMessage message = LoginFailureMessage::deserialize(payload);
        
        UI::printErrorMessage("Đăng nhập thất bại: " + message.error_message);
        UI::displayInitialMenuPrompt();
        return ClientState::INITIAL_MENU;
    }

    // ==================== Game handlers ====================

    ClientState handleGameStart(const std::vector<uint8_t> &payload)
    {
        GameStartMessage message = GameStartMessage::deserialize(payload);
        SessionData &session = SessionData::getInstance();
        
        UI::clearConsole();
        UI::displayGameStart(message.game_id, message.player1_username, 
                            message.player2_username, message.starting_player_username);

        bool is_white = (message.starting_player_username == session.getUsername());
        session.setGameStatus(message.game_id, is_white, message.fen);

        UI::showBoard(message.fen, !is_white);
        
        if (is_white)
        {
            UI::displayMovePrompt();
            return ClientState::IN_GAME_MY_TURN;
        }
        else
        {
            UI::displayWaitingOpponentMove();
            return ClientState::IN_GAME_OPPONENT_TURN;
        }
    }

    ClientState handleGameStatusUpdate(const std::vector<uint8_t> &payload)
    {
        GameStatusUpdateMessage message = GameStatusUpdateMessage::deserialize(payload);
        SessionData &session = SessionData::getInstance();

        session.setFen(message.fen);
        bool is_my_turn = (message.current_turn_username == session.getUsername());
        session.setTurn(is_my_turn);

        UI::showBoard(message.fen, !session.isWhite());

        if (message.is_game_over)
        {
            // Game over will be handled by GAME_END message
            return ClientState::IN_GAME_OPPONENT_TURN;
        }

        if (is_my_turn)
        {
            UI::displayMovePrompt();
            return ClientState::IN_GAME_MY_TURN;
        }
        else
        {
            UI::displayWaitingOpponentMove();
            return ClientState::IN_GAME_OPPONENT_TURN;
        }
    }

    ClientState handleInvalidMove(const std::vector<uint8_t> &payload)
    {
        InvalidMoveMessage message = InvalidMoveMessage::deserialize(payload);
        
        UI::printErrorMessage("Nước đi không hợp lệ: " + message.error_message);
        UI::displayMovePrompt();
        return ClientState::IN_GAME_MY_TURN;
    }

    ClientState handleGameEnd(const std::vector<uint8_t> &payload)
    {
        GameEndMessage message = GameEndMessage::deserialize(payload);
        SessionData &session = SessionData::getInstance();
        
        UI::clearConsole();
        UI::displayGameEnd(message.game_id, message.winner_username, 
                          message.reason, message.half_moves_count);
        
        session.clearGameStatus();
        UI::displayGameMenuPrompt();
        return ClientState::GAME_MENU;
    }

    // ==================== Matchmaking handlers ====================

    ClientState handleAutoMatchFound(const std::vector<uint8_t> &payload, StateContext &context)
    {
        AutoMatchFoundMessage message = AutoMatchFoundMessage::deserialize(payload);
        
        context.pending_game_id = message.game_id;
        context.opponent_username = message.opponent_username;
        context.opponent_elo = message.opponent_elo;
        
        UI::clearConsole();
        UI::displayAutoMatchOptionsPrompt(message.opponent_username, 
                                          message.opponent_elo, message.game_id);
        return ClientState::AUTO_MATCH_DECISION;
    }

    ClientState handleMatchDeclinedNotification(const std::vector<uint8_t> &payload)
    {
        MatchDeclinedNotificationMessage message = MatchDeclinedNotificationMessage::deserialize(payload);
        
        UI::printInfoMessage("Đối thủ đã từ chối trận đấu.");
        UI::displayGameMenuPrompt();
        return ClientState::GAME_MENU;
    }

    // ==================== Challenge handlers ====================

    ClientState handlePlayerList(const std::vector<uint8_t> &payload, StateContext &context)
    {
        PlayerListMessage message = PlayerListMessage::deserialize(payload);
        SessionData &session = SessionData::getInstance();
        
        context.player_list_cache = message.players;
        UI::clearConsole();
        UI::displayPlayerList(message.players, session.getUsername());
        return ClientState::PLAYER_LIST_VIEW;
    }

    ClientState handleChallengeNotification(const std::vector<uint8_t> &payload, StateContext &context)
    {
        ChallengeNotificationMessage message = ChallengeNotificationMessage::deserialize(payload);
        
        context.challenger_username = message.from_username;
        context.challenger_elo = message.elo;
        
        UI::clearConsole();
        UI::displayChallengeDecisionPrompt(message.from_username, message.elo);
        return ClientState::CHALLENGE_RECEIVED;
    }

    ClientState handleChallengeDeclined(const std::vector<uint8_t> &payload)
    {
        ChallengeDeclinedMessage message = ChallengeDeclinedMessage::deserialize(payload);
        
        UI::printInfoMessage("Thách đấu đã bị từ chối bởi " + message.from_username);
        UI::displayGameMenuPrompt();
        return ClientState::GAME_MENU;
    }

    ClientState handleChallengeAccepted(const std::vector<uint8_t> &payload)
    {
        ChallengeAcceptedMessage message = ChallengeAcceptedMessage::deserialize(payload);
        
        UI::printInfoMessage("Thách đấu đã được chấp nhận! Đang bắt đầu trận...");
        // GAME_START message will follow
        return ClientState::WAITING_MATCH_START;
    }

    ClientState handleChallengeError(const std::vector<uint8_t> &payload)
    {
        ChallengeErrorMessage message = ChallengeErrorMessage::deserialize(payload);
        
        UI::printErrorMessage("Lỗi thách đấu: " + message.error_message);
        UI::displayGameMenuPrompt();
        return ClientState::GAME_MENU;
    }
};

#endif // MESSAGE_HANDLER_HPP
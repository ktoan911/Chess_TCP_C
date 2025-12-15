#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include <iomanip>      // Optional if using std::put_time
#include <sstream> 
#include <chrono>

#include "../common/protocol.hpp"
#include "../common/message.hpp"

#include "network_client.hpp"
#include "session_data.hpp"
#include "logic_handler.hpp"
#include "input_handler.hpp"
#include "ui.hpp"

class MessageHandler
{
public:
    /**
     * @brief Đẩy một gói tin mới vào hàng đợi xử lý
     *
     * Tạo một luồng mới để xử lý gói tin và chạy ngầm
     * @param packet Gói tin cần xử lý
     * @return true nếu đẩy thành công
     */
    bool pushMessage(const Packet &packet)
    {
        SessionData &session_data = SessionData::getInstance();
        // Start new handler thread
        std::thread([this, packet, &session_data]()
                    {
            {
                session_data.setCurrentHandler(std::this_thread::get_id());
            }

            handleMessage(packet);
            
            if (session_data.isCurrentHandler()) {
                session_data.setCurrentHandler(std::thread::id());
            } })
            .detach();

        return true;
    }

private:
    // Handle incoming messages ================================================================================

    /**
     * @brief Xử lý thông điệp dựa trên loại của gói tin.
     *
     * @param packet Gói tin chứa thông điệp cần xử lý.
     * @return true nếu xử lý thành công, false nếu không xác định được loại thông điệp.
     */
    bool handleMessage(const Packet &packet)
    {
        bool success = true;
        switch (packet.type)
        {
        case MessageType::REGISTER_SUCCESS:
            // Handle register success
            handleRegisterSuccess(packet.payload);
            break;
        case MessageType::REGISTER_FAILURE:
            // Handle register failure
            handleRegisterFailure(packet.payload);
            break;
        case MessageType::LOGIN_SUCCESS:
            // Handle login success
            handleLoginSuccess(packet.payload);
            break;
        case MessageType::LOGIN_FAILURE:
            // Handle login failure
            handleLoginFailure(packet.payload);
            break;

        case MessageType::GAME_START:
            // Handle game start
            handleGameStart(packet.payload);
            break;
        case MessageType::GAME_STATUS_UPDATE:
            // Handle game status update
            handleGameStatusUpdate(packet.payload);
            break;
        case MessageType::INVALID_MOVE:
            // Handle invalid move
            handleInvalidMove(packet.payload);
            break;
        case MessageType::GAME_END:
            // Handle game end
            handleGameEnd(packet.payload);
            break;

        case MessageType::CHALLENGE_NOTIFICATION:
            // Handle challenge notification
            handleChallengeNotification(packet.payload);
            break;

        case MessageType::AUTO_MATCH_FOUND:
            // Handle auto match found
            handleAutoMatchFound(packet.payload);
            break;
        case MessageType::MATCH_DECLINED_NOTIFICATION:
            // Handle match declined notification
            handleMatchDeclinedNotification(packet.payload);
            break;

            // Add additional cases for other MessageTypes

        case MessageType::PLAYER_LIST:
            // Handle player list received from server
            handlePlayerList(packet.payload);
            break;
        case MessageType::CHALLENGE_DECLINED:
            // Handle challenge declined
            handleChallengeDeclined(packet.payload);
            break;
        case MessageType::CHALLENGE_ACCEPTED:
            // Handle challenge accepted
            handleChallengeAccepted(packet.payload);
            break;

        case MessageType::SPECTATE_SUCCESS:
            // Handle spectate success
            handleSpectateSuccess(packet.payload);
            break;
        case MessageType::SPECTATE_FAILURE:
            // Handle spectate failure
            handleSpectateFailure(packet.payload);
            break;
        case MessageType::SPECTATE_END:
            // Handle spectate end
            handleSpectateEnd(packet.payload);
            break;
        case MessageType::SPECTATE_MOVE:
            // Handle spectate move
            handleSpectateMove(packet.payload);
            break;

        case MessageType::MATCH_HISTORY:
            // Handle match history
            handleMatchHistory(packet.payload);
            break;

        default:
            // Handle unknown message type
            handleUnknown(packet.payload);
            success = false;
            break;
        }

        return success;
    }

    // Handle specific message types ============================================================================

    void handleUnknown(const std::vector<uint8_t> &payload)
    {
        std::cout << "[UNKNOWN]" << std::endl;
    }

    void handleRegisterSuccess(const std::vector<uint8_t> &payload)
    {
        RegisterSuccessMessage message = RegisterSuccessMessage::deserialize(payload);

        UI::printSuccessMessage("Đăng ký thành công.");
        std::cout << "Username: " << message.username << "\n"
                  << "ELO: " << message.elo << std::endl;

        // Update session data
        SessionData &session_data = SessionData::getInstance();

        session_data.setUsername(message.username);
        session_data.setElo(message.elo);

        // Display the game menu
        LogicHandler logic_handler;
        logic_handler.handleGameMenu();
    }

    void handleRegisterFailure(const std::vector<uint8_t> &payload)
    {
        RegisterFailureMessage message = RegisterFailureMessage::deserialize(payload);

        UI::printErrorMessage("Đăng ký thất bại.");
        std::cout << "Error_message: " << message.error_message << std::endl;
    }

    void handleLoginSuccess(const std::vector<uint8_t> &payload)
    {
        LoginSuccessMessage message = LoginSuccessMessage::deserialize(payload);

        UI::printSuccessMessage("Đăng nhập thành công.");
        std::cout << "Username: " << message.username << "\n"
                  << "ELO: " << message.elo << std::endl;

        // Update session data
        SessionData &session_data = SessionData::getInstance();

        session_data.setUsername(message.username);
        session_data.setElo(message.elo);

        // Display the game menu
        LogicHandler logic_handler;
        logic_handler.handleGameMenu();
    }

    void handleLoginFailure(const std::vector<uint8_t> &payload)
    {
        LoginFailureMessage message = LoginFailureMessage::deserialize(payload);

        UI::printErrorMessage("Đăng nhập thất bại.");
        std::cout << "Error_message: " << message.error_message << std::endl;

        LogicHandler logic_handler;
        logic_handler.handleInitialMenu();
    }

    void handleGameStart(const std::vector<uint8_t> &payload)
    {
        GameStartMessage message = GameStartMessage::deserialize(payload);

        // Display the game menu
        LogicHandler logic_handler;
        logic_handler.handleGameStart(message);
    }

    void handleGameStatusUpdate(const std::vector<uint8_t> &payload)
    {
        GameStatusUpdateMessage message = GameStatusUpdateMessage::deserialize(payload);

        // Handle game status update
        LogicHandler logic_handler;
        logic_handler.handleGameStatusUpdate(message);
    }

    void handleInvalidMove(const std::vector<uint8_t> &payload)
    {
        InvalidMoveMessage message = InvalidMoveMessage::deserialize(payload);

        UI::printErrorMessage("Nước đi không hợp lệ.");
        std::cout << "Game_id: " << message.game_id << "\n"
                  << "Error_message: " << message.error_message << std::endl;

        LogicHandler logic_handler;
        logic_handler.handleMove();
    }

    void handleGameEnd(const std::vector<uint8_t> &payload)
    {
        GameEndMessage message = GameEndMessage::deserialize(payload);

        UI::printInfoMessage("Trò chơi đã kết thúc.");
        std::cout << "Game_id: " << message.game_id << "\n"
                  << "Winner: " << message.winner_username << "\n"
                  << "Reason: " << message.reason << "\n"
                  << "Half_moves_count: " << message.half_moves_count
                  << std::endl;

        // print the menu after game ends
        LogicHandler logic_handler;
        logic_handler.handleGameMenu();
    }

    void handleAutoMatchFound(const std::vector<uint8_t> &payload)
    {
        AutoMatchFoundMessage message = AutoMatchFoundMessage::deserialize(payload);
        LogicHandler logic_handler;
        logic_handler.handleMatchDecision(message);
    }

    void handleMatchDeclinedNotification(const std::vector<uint8_t> &payload)
    {
        MatchDeclinedNotificationMessage message = MatchDeclinedNotificationMessage::deserialize(payload);

        UI::printInfoMessage("Trận đấu đã bị từ chối.");
        std::cout << "Game_id: " << message.game_id << std::endl;

        LogicHandler logic_handler;
        logic_handler.handleGameMenu();
    }

    void handlePlayerList(const std::vector<uint8_t> &payload)
    {
        PlayerListMessage message = PlayerListMessage::deserialize(payload);

        for (const auto &player : message.players)
        {
            std::cout << "Username: " << player.username << "\n"
                      << "ELO: " << player.elo << std::endl;
        }

        LogicHandler logic_handler;

        logic_handler.handlePlayerListDecision(message.players);

        SessionData &session_data = SessionData::getInstance();
        if (session_data.shouldStop())
        {
            // std::cout << "shouldStop..." << std::endl;
            return;
        }
        logic_handler.handleGameMenu();
    }

    void handleChallengeNotification(const std::vector<uint8_t> &payload)
    {
        ChallengeNotificationMessage message = ChallengeNotificationMessage::deserialize(payload);

        UI::printInfoMessage("Nhận được thách đấu từ người chơi khác.");
        std::cout << "Challenger: " << message.from_username << std::endl;
        std::cout << "ELO: " << message.elo << std::endl;

        LogicHandler logic_handler;
        logic_handler.handleChallengeDecision(message.from_username);
    }

    void handleChallengeDeclined(const std::vector<uint8_t> &payload)
    {
        ChallengeDeclinedMessage message = ChallengeDeclinedMessage::deserialize(payload);

        UI::printInfoMessage("Thách đấu đã bị từ chối.");

        LogicHandler logic_handler;
        logic_handler.handleGameMenu();
    }

    void handleChallengeAccepted(const std::vector<uint8_t> &payload)
    {
        ChallengeAcceptedMessage message = ChallengeAcceptedMessage::deserialize(payload);

        UI::printInfoMessage("Thách đấu đã được chấp nhận.");
    }

    void handleSpectateSuccess(const std::vector<uint8_t> &payload)
    {
        NetworkClient &network_client = NetworkClient::getInstance();
        SpectateSuccessMessage message = SpectateSuccessMessage::deserialize(payload);
        std::string game_id = message.game_id;

        UI::printInfoMessage("Bắt đầu quan sát trận đấu.");

        std::cout << "Nhập Enter để kết thúc quan sát." << std::endl;

        // Blocking
        std::string input = InputHandler::waitForInput();

        SpectateExitMessage spectate_exit_msg;
        spectate_exit_msg.game_id = game_id;
        network_client.sendPacket(spectate_exit_msg.getType(), spectate_exit_msg.serialize());

        LogicHandler logic_handler;
        logic_handler.handleGameMenu();
    }

    void handleSpectateFailure(const std::vector<uint8_t> &payload)
    {
        SpectateFailureMessage message = SpectateFailureMessage::deserialize(payload);

        UI::printErrorMessage("Người chơi này hiện không trong trận đấu nào.");

        LogicHandler logic_handler;
        logic_handler.handleGameMenu();
    }

    void handleSpectateEnd(const std::vector<uint8_t> &payload)
    {
        LogicHandler logic_handler;

        SpectateEndMessage message = SpectateEndMessage::deserialize(payload);

        UI::printInfoMessage("Trận đấu bạn quan sát đã kết thúc.");

        logic_handler.handleGameMenu();
    }

    void handleSpectateMove(const std::vector<uint8_t> &payload)
    {
        SpectateMoveMessage message = SpectateMoveMessage::deserialize(payload);

        UI::showBoard(message.fen);
        std::cout << "Tiếp theo sẽ đến lượt của: " << message.current_turn_username
                  << " - " << ((message.is_white) ? "Trắng" : "Đen") << std::endl;
        std::cout << "Nhập Enter để kết thúc quan sát." << std::endl;
    }

    void handleMatchHistory(const std::vector<uint8_t> &payload)
    {
        MatchHistoryMessage message = MatchHistoryMessage::deserialize(payload);

        UI::printInfoMessage("Lịch sử trận đấu:");

        for (const auto &match : message.matches)
        {
            std::cout << "Game_id: " << match.game_id << "\n"
                      << "Opponent: " << match.opponent_username << "\n"
                      << "Result: " << (match.won ? "Win" : "Lose") << "\n"
                      << "Time: " << match.date << "\n"
                      << "-------------------------------------------" << std::endl;
        }

        LogicHandler logic_handler;
        logic_handler.handleMatchHistoryDecision();
    }

}; // namespace MessageHandler

#endif // MESSAGE_HANDLER_HPP
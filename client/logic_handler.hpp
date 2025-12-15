#ifndef LOGIC_HANDLER_HPP
#define LOGIC_HANDLER_HPP

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

#include "../common/protocol.hpp"
#include "../common/message.hpp"

#include "session_data.hpp"
#include "network_client.hpp"
#include "ui.hpp"

class MessageHandler;

class LogicHandler
{
public:
    /**
     * @brief Xử lý menu ban đầu của ứng dụng, cho phép người dùng đăng ký, đăng nhập hoặc thoát.
     *
     * Hàm này hiển thị logo ứng dụng và trình bày menu ban đầu cho người dùng.
     * Dựa trên lựa chọn của người dùng, nó thực hiện các hành động sau:
     *
     * - Đăng ký (Lựa chọn 1): Yêu cầu người dùng nhập tên đăng nhập, tạo một RegisterMessage,
     *   và gửi nó đến máy chủ. Nếu gửi thất bại, một thông báo lỗi được hiển thị.
     *
     * - Đăng nhập (Lựa chọn 2): Yêu cầu người dùng nhập tên đăng nhập, tạo một LoginMessage,
     *   và gửi nó đến máy chủ. Nếu gửi thất bại, một thông báo lỗi được hiển thị.
     *
     * - Thoát (Lựa chọn 3): Đặt cờ running thành false và đóng kết nối mạng.
     *
     * Nếu người dùng nhập một lựa chọn không hợp lệ, một thông báo thích hợp được hiển thị.
     *
     * @param running Một tham chiếu đến một biến boolean nguyên tử điều khiển trạng thái chạy của ứng dụng.
     */
    void handleInitialMenu()
    {
        UI::printLogo();

        NetworkClient &network_client = NetworkClient::getInstance();
        SessionData &session_data = SessionData::getInstance();

        while (true)
        {
            std::string result = UI::displayInitialMenu();

            // if the input was cancelled
            if (session_data.shouldStop())
            {
                // std::cout << "shouldStop..." << std::endl;
                break;
            }

            int choice;
            try
            {
                choice = std::stoi(result);
            }
            catch (const std::exception &e)
            {
                UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn lại.");
                continue;
            }

            if (choice == 1)
            {
                std::string username = UI::displayRegister();

                RegisterMessage reg_msg = RegisterMessage{username};

                if (!network_client.sendPacket(reg_msg.getType(), reg_msg.serialize()))
                {
                    UI::printErrorMessage("Gửi yêu cầu đăng ký thất bại.");
                    break;
                }
                break;
            }
            else if (choice == 2)
            {
                std::string username = UI::displayLogin(network_client);

                LoginMessage login_msg = LoginMessage{username};

                if (!network_client.sendPacket(login_msg.getType(), login_msg.serialize()))
                {
                    UI::printErrorMessage("Gửi yêu cầu đăng nhập thất bại.");
                    break;
                }
                break;
            }
            else if (choice == 3)
            {
                session_data.setRunning(false);
                network_client.closeConnection();
                break;
            }
            else
            {
                UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn lại.");
            }
        }
    }

    /**
     * Xử lý menu trò chơi, bao gồm gửi yêu cầu ghép trận tự động, chơi với máy, hoặc trở về menu chính.
     */
    void handleGameMenu()
    {
        NetworkClient &network_client = NetworkClient::getInstance();
        SessionData &session_data = SessionData::getInstance();

        while (true)
        {
            std::string result = UI::displayGameMenu();

            // if result is empty, it means the input was cancelled
            if (session_data.shouldStop())
            {
                // std::cout << "shouldStop..." << std::endl;
                break;
            }

            int choice;
            try
            {
                choice = std::stoi(result);
            }
            catch (const std::exception &e)
            {
                UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn lại.");
                continue;
            }

            if (choice == 1)
            {
                // Gửi yêu cầu tìm đối thủ
                AutoMatchRequestMessage auto_match_request_msg;
                auto_match_request_msg.username = session_data.getUsername();

                if (!network_client.sendPacket(auto_match_request_msg.getType(), auto_match_request_msg.serialize()))
                {
                    UI::printErrorMessage("Gửi yêu cầu ghép trận tự động thất bại.");
                    break;
                }

                UI::printInfoMessage("Đang tìm đối thủ...");
                break;
            }
            else if (choice == 2)
            {
                // Chơi với máy
                PlayWithBotMessage play_with_bot_msg;
                play_with_bot_msg.username = session_data.getUsername();

                if (!network_client.sendPacket(play_with_bot_msg.getType(), play_with_bot_msg.serialize()))
                {
                    UI::printErrorMessage("Gửi yêu cầu chơi với máy thất bại.");
                }

                UI::printInfoMessage("Đang chơi với máy...");
                break;
            }
            else if (choice == 3)
            {
                // Xem danh sách người chơi trực tuyến
                RequestPlayerListMessage request_player_list_msg;
                if (!network_client.sendPacket(request_player_list_msg.getType(), request_player_list_msg.serialize()))
                {
                    UI::printErrorMessage("Tải danh sách người chơi trực tuyến thất bại.");
                    break;
                }
                UI::printInfoMessage("Danh sách người chơi trực tuyến:");
                break;
            }
            else if (choice == 4)
            {
                // Xem lịch sử trận đấu
                RequestMatchHistoryMessage request_match_history_msg;

                if (!network_client.sendPacket(request_match_history_msg.getType(), request_match_history_msg.serialize()))
                {
                    UI::printErrorMessage("Tải lịch sử trận đấu thất bại.");
                    break;
                }
                break;
            }
            else if (choice == 5)
            {
                handleInitialMenu();
                break;
            }
            else
            {
                UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn lại.");
                break;
            }
        }
    }

    void handleMatchDecision(AutoMatchFoundMessage &message)
    {
        UI::printSuccessMessage("Tìm thấy trận!");
        std::cout << "Opponent_username: " << message.opponent_username << "\n"
                  << "Opponent_elo: " << message.opponent_elo << "\n"
                  << "Game_id: " << message.game_id << std::endl;

        NetworkClient &network_client = NetworkClient::getInstance();

        while (true)
        {
            std::string result = UI::displayAutoMatchOptions();

            // if the input was cancelled
            if (SessionData::getInstance().shouldStop())
            {
                // std::cout << "shouldStop..." << std::endl;
                break;
            }

            int choice;
            try
            {
                choice = std::stoi(result);
            }
            catch (const std::exception &e)
            {
                UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn lại.");
                continue;
            }

            if (choice == 1)
            {
                // Chấp nhận
                AutoMatchAcceptedMessage auto_match_accepted_msg;
                auto_match_accepted_msg.game_id = message.game_id;

                if (!network_client.sendPacket(auto_match_accepted_msg.getType(), auto_match_accepted_msg.serialize()))
                {
                    UI::printErrorMessage("Gửi yêu cầu chấp nhận ghép trận tự động thất bại.");
                    break;
                }

                UI::printInfoMessage("Đã chấp nhận ghép trận tự động.");
                break;
            }
            else if (choice == 2)
            {
                // Từ chối
                AutoMatchDeclinedMessage auto_match_declined_msg;
                auto_match_declined_msg.game_id = message.game_id;

                if (!network_client.sendPacket(auto_match_declined_msg.getType(), auto_match_declined_msg.serialize()))
                {
                    UI::printErrorMessage("Gửi yêu cầu từ chối ghép trận tự động thất bại.");
                    break;
                }

                UI::printInfoMessage("Đã từ chối ghép trận tự động.");

                handleGameMenu();
                break;
            }
            else
            {
                UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn lại.");
                break;
            }
        }
    }

    void handleGameStart(const GameStartMessage &message)
    {
        UI::printInfoMessage("Trò chơi đã bắt đầu.");
        std::cout << "Game_id: " << message.game_id << "\n"
                  << "Player_1: " << message.player1_username << "\n"
                  << "Player_2: " << message.player2_username << "\n"
                  << "Starting player: " << message.starting_player_username << "\n"
                  << "FEN: " << message.fen << std::endl;

        // Set game status
        SessionData &session_data = SessionData::getInstance();

        bool is_white = message.starting_player_username == session_data.getUsername();
        session_data.setGameStatus(message.game_id, is_white, message.fen);

        // Bắt đầu trò chơi
        handleMove();
    }

    void handleGameStatusUpdate(const GameStatusUpdateMessage &message)
    {
        UI::printInfoMessage("Trò chơi đã cập nhật.");
        std::cout << "Game_id: " << message.game_id << std::endl
                  << "FEN: " << message.fen << std::endl
                  << "Current_turn_username: " << message.current_turn_username << std::endl
                  << "Is_game_over: " << static_cast<bool>(message.is_game_over) << std::endl;

        // Update session data
        SessionData &session_data = SessionData::getInstance();

        bool is_my_turn = message.current_turn_username == session_data.getUsername();

        session_data.setTurn(is_my_turn);
        session_data.setFen(message.fen);

        if (static_cast<bool>(message.is_game_over))
        {
            UI::showBoard(session_data.getFen(), !session_data.isWhite());
            return;
        }

        handleMove();
    }

    void handleMove()
    {
        SessionData &session_data = SessionData::getInstance();
        NetworkClient &network_client = NetworkClient::getInstance();

        UI::showBoard(session_data.getFen(), !session_data.isWhite());

        if (session_data.isMyTurn())
        {
            // Lượt của người chơi
            std::string result = UI::getMove();

            if (session_data.shouldStop())
            {
                // std::cout << "shouldStop..." << std::endl;
                return;
            }

            if (result == "surrender")
            {
                // Gửi thông điệp đầu hàng
                SurrenderMessage surrender_msg;
                surrender_msg.game_id = session_data.getGameId();
                surrender_msg.from_username = session_data.getUsername();

                if (!network_client.sendPacket(surrender_msg.getType(), surrender_msg.serialize()))
                {
                    UI::printErrorMessage("Gửi thông điệp đầu hàng thất bại.");
                    return;
                }
                UI::printInfoMessage("Bạn đã đầu hàng!");
                return;
            }

            std::string uci_move = result;

            // Gửi nước đi
            MoveMessage move_msg;
            move_msg.game_id = session_data.getGameId();
            move_msg.uci_move = uci_move;

            if (!network_client.sendPacket(move_msg.getType(), move_msg.serialize()))
            {
                UI::printErrorMessage("Gửi nước đi thất bại.");
                return;
            }
        }
        else
        {
            // Lượt của đối thủ
            UI::printInfoMessage("Đang chờ đối thủ ra nước đi...");
        }
    }
    void handlePlayerListDecision(std::vector<PlayerListMessage::Player> &players)
    {
        NetworkClient &network_client = NetworkClient::getInstance();
        SessionData &session_data = SessionData::getInstance();

        /* Ask the user to choose an option
         * 1. Challenge a player
         * 2. Watch a player playing an ongoing match
         * 3. Go back
         */
        UI::PlayerListDecision decision = UI::displayPlayerListOption();

        /* Logic check */

        bool userOnline = false;

        ChallengeRequestMessage challenge_request_msg;
        RequestSpectateMessage request_spectate_msg;

        switch (decision.choice)
        {
        case 1: /* Thách đấu người khác */
            // Kiểm tra username có trùng với bản thân không
            if (decision.username == session_data.getUsername())
            {
                UI::printErrorMessage("Không thể thách đấu với chính mình.");
                handleGameMenu();
                return;
            }

            // Kiểm tra người chơi có online không
            for (const auto &player : players)
            {
                if (player.username == decision.username)
                {
                    if (player.in_game)
                    {
                        UI::printErrorMessage("Người chơi đang trong trận đấu.");
                        handleGameMenu();
                        return;
                    }
                    userOnline = true;
                    break;
                }
            }

            if (!userOnline)
            {
                UI::printErrorMessage("Người chơi không online hoặc không tồn tại.");
                handleGameMenu();
                return;
            }

            challenge_request_msg.to_username = decision.username;

            if (!network_client.sendPacket(challenge_request_msg.getType(), challenge_request_msg.serialize()))
            {
                UI::printErrorMessage("Gửi yêu cầu thách đấu thất bại.");
                handleGameMenu();
            }
            else
            {
                UI::printInfoMessage("Đã gửi yêu cầu thách đấu. Đang chờ phản hồi...");
                std::this_thread::sleep_for(std::chrono::seconds(10));
                // if the input was cancelled
                if (SessionData::getInstance().shouldStop())
                {
                    // std::cout << "shouldStop..." << std::endl;
                    return;
                }
                UI::printInfoMessage("Hết thời gian chờ đợi.");
                handleGameMenu();
            }
            break;
        case 2: /*Xem người chơi khác chơi*/
            // Kiểm tra xem có định xem chính mình chơi không
            if (decision.username == session_data.getUsername())
            {
                UI::printErrorMessage("Không thể xem chính mình chơi.");
                handleGameMenu();
                return;
            }
            // Kiểm tra người chơi có online không
            for (const auto &player : players)
            {
                if (player.username == decision.username)
                {
                    if (!player.in_game)
                    {
                        UI::printErrorMessage("Người chơi này hiện không trong trận đấu nào.");
                        handleGameMenu();
                        return;
                    }
                    userOnline = true;
                    break;
                }
            }

            if (!userOnline)
            {
                UI::printErrorMessage("Người chơi không online hoặc không tồn tại.");
                handleGameMenu();
                return;
            }

            request_spectate_msg.username = decision.username;
            if (!network_client.sendPacket(request_spectate_msg.getType(), request_spectate_msg.serialize()))
            {
                UI::printErrorMessage("Gửi yêu cầu xem trận thất bại.");
                handleGameMenu();
            }
            else
            {
                UI::printInfoMessage("Đã gửi yêu cầu xem trận.");
            }
            break;
        case 3:
            handleGameMenu();
            break;
        default:
            handleGameMenu();
            break;
        }
    }

    void handleChallengeDecision(std::string challenger_username)
    {
        NetworkClient &network_client = NetworkClient::getInstance();
        SessionData &session_data = SessionData::getInstance();

        while (true)
        {
            std::string result = UI::displayChallengeDecision();

            // if the input was cancelled
            if (SessionData::getInstance().shouldStop())
            {
                // std::cout << "shouldStop..." << std::endl;
                break;
            }

            int choice;

            if (result == "<TIMEOUT>")
            {
                UI::printErrorMessage("Hết thời gian chờ đợi. Tự động từ chối thách đấu.");
                choice = 2;
            }
            else
            {
                try
                {
                    choice = std::stoi(result);
                }
                catch (const std::exception &e)
                {
                    UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn lại.");
                    continue;
                }
            }

            // create new response to challenge (accept = 1 /decline = 0)
            // this message will be used in both cases below

            ChallengeResponseMessage challenge_response_msg;
            challenge_response_msg.from_username = challenger_username;

            if (choice == 1)
            {
                // Chấp nhận
                // ChallengeAcceptedMessage challenge_accepted_msg;

                challenge_response_msg.response = ChallengeResponseMessage::Response::ACCEPTED;

                if (!network_client.sendPacket(challenge_response_msg.getType(), challenge_response_msg.serialize()))
                {
                    UI::printErrorMessage("Gửi yêu cầu chấp nhận thách đấu thất bại.");
                    break;
                }

                UI::printInfoMessage("Đã chấp nhận thách đấu.");
                break;
            }
            else if (choice == 2)
            {
                // Từ chối
                // ChallengeDeclinedMessage challenge_declined_msg;

                challenge_response_msg.response = ChallengeResponseMessage::Response::DECLINED;

                if (!network_client.sendPacket(challenge_response_msg.getType(), challenge_response_msg.serialize()))
                {
                    UI::printErrorMessage("Gửi yêu cầu từ chối thách đấu thất bại.");
                    break;
                }

                UI::printInfoMessage("Đã từ chối thách đấu.");
                handleGameMenu();
                break;
            }
            else
            {
                UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn lại.");
                break;
            }
        }
    }

    void handleMatchHistoryDecision()
    {
        std::string result = UI::displayMatchHistoryDecision();

        // if the input was cancelled
        if (SessionData::getInstance().shouldStop())
        {
            // std::cout << "shouldStop..." << std::endl;
            return;
        }

        if (result == "2")
        {
            handleGameMenu();
            return;
        }

        // Xem lại trận đấu
        std::string game_id = result;

        // RequestMatchDataMessage request_match_data_msg;
        // request_match_data_msg.game_id = game_id;

        // NetworkClient &network_client = NetworkClient::getInstance();

        // if (!network_client.sendPacket(request_match_data_msg.getType(), request_match_data_msg.serialize()))
        // {
        //     UI::printErrorMessage("Gửi yêu cầu xem lại trận đấu thất bại.");
        //     handleGameMenu();
        // }
        // else
        // {
        //     UI::printInfoMessage("Đã gửi yêu cầu xem lại trận đấu.");
        // }
    }

private:
}; // class LogicHandler

#endif // LOGIC_HANDLER_HPP

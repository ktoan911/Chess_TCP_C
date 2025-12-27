#ifndef INPUT_PROCESSOR_HPP
#define INPUT_PROCESSOR_HPP

#include <string>
#include <iostream>

#include "../common/protocol.hpp"
#include "../common/message.hpp"

#include "client_state.hpp"
#include "session_data.hpp"
#include "network_client.hpp"
#include "ui.hpp"

/**
 * @brief Xử lý input từ user một cách đồng bộ
 * 
 * Thay thế logic_handler.hpp - xử lý input dựa trên state hiện tại
 * và trả về state mới cho event loop.
 */
class InputProcessor
{
public:
    /**
     * @brief Xử lý input từ user và trả về state mới
     * 
     * @param currentState State hiện tại
     * @param input Input string từ user
     * @param context State context để lưu data tạm
     * @return ClientState mới sau khi xử lý
     */
    ClientState processInput(ClientState currentState, const std::string &input, StateContext &context)
    {
        // Empty input, keep current state
        if (input.empty())
        {
            redisplayPrompt(currentState, context);
            return currentState;
        }

        switch (currentState)
        {
        case ClientState::INITIAL_MENU:
            return processInitialMenu(input);
            
        case ClientState::WAITING_REGISTER_INPUT:
            return processRegisterInput(input);
            
        case ClientState::WAITING_LOGIN_INPUT:
            return processLoginInput(input);
            
        case ClientState::GAME_MENU:
            return processGameMenu(input);
            
        case ClientState::AUTO_MATCH_DECISION:
            return processAutoMatchDecision(input, context);
            
        case ClientState::PLAYER_LIST_VIEW:
            return processPlayerListView(input, context);
            
        case ClientState::CHALLENGE_INPUT:
            return processChallengeInput(input, context);
            
        case ClientState::CHALLENGE_RECEIVED:
            return processChallengeReceived(input, context);
            
        case ClientState::IN_GAME_MY_TURN:
            return processGameMove(input);
            
        // States that don't accept input - waiting for server
        case ClientState::WAITING_REGISTER_RESPONSE:
        case ClientState::WAITING_LOGIN_RESPONSE:
        case ClientState::WAITING_AUTO_MATCH:
        case ClientState::WAITING_MATCH_START:
        case ClientState::WAITING_PLAYER_LIST:
        case ClientState::WAITING_CHALLENGE_RESPONSE:
        case ClientState::IN_GAME_OPPONENT_TURN:
            // Ignore input in waiting states
            return currentState;
            
        case ClientState::EXITING:
            return ClientState::EXITING;
            
        default:
            return currentState;
        }
    }

private:
    NetworkClient& network_ = NetworkClient::getInstance();
    SessionData& session_ = SessionData::getInstance();

    // ==================== Initial Menu ====================

    ClientState processInitialMenu(const std::string &input)
    {
        int choice = 0;
        try
        {
            choice = std::stoi(input);
        }
        catch (...)
        {
            UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn 1, 2, hoặc 3.");
            UI::displayInitialMenuPrompt();
            return ClientState::INITIAL_MENU;
        }

        switch (choice)
        {
        case 1: // Register
            UI::displayRegisterPrompt();
            return ClientState::WAITING_REGISTER_INPUT;
            
        case 2: // Login
            UI::displayLoginPrompt();
            return ClientState::WAITING_LOGIN_INPUT;
            
        case 3: // Exit
            UI::printInfoMessage("Tạm biệt!");
            return ClientState::EXITING;
            
        default:
            UI::printErrorMessage("Lựa chọn không hợp lệ. Vui lòng chọn 1, 2, hoặc 3.");
            UI::displayInitialMenuPrompt();
            return ClientState::INITIAL_MENU;
        }
    }

    ClientState processRegisterInput(const std::string &input)
    {
        if (input.empty())
        {
            UI::printErrorMessage("Username không được để trống.");
            UI::displayRegisterPrompt();
            return ClientState::WAITING_REGISTER_INPUT;
        }

        RegisterMessage msg;
        msg.username = input;
        
        if (!network_.sendPacket(msg.getType(), msg.serialize()))
        {
            UI::printErrorMessage("Gửi yêu cầu đăng ký thất bại.");
            UI::displayInitialMenuPrompt();
            return ClientState::INITIAL_MENU;
        }

        UI::printInfoMessage("Đang xử lý đăng ký...");
        return ClientState::WAITING_REGISTER_RESPONSE;
    }

    ClientState processLoginInput(const std::string &input)
    {
        if (input.empty())
        {
            UI::printErrorMessage("Username không được để trống.");
            UI::displayLoginPrompt();
            return ClientState::WAITING_LOGIN_INPUT;
        }

        LoginMessage msg;
        msg.username = input;
        
        if (!network_.sendPacket(msg.getType(), msg.serialize()))
        {
            UI::printErrorMessage("Gửi yêu cầu đăng nhập thất bại.");
            UI::displayInitialMenuPrompt();
            return ClientState::INITIAL_MENU;
        }

        UI::printInfoMessage("Đang xử lý đăng nhập...");
        return ClientState::WAITING_LOGIN_RESPONSE;
    }

    // ==================== Game Menu ====================

    ClientState processGameMenu(const std::string &input)
    {
        int choice = 0;
        try
        {
            choice = std::stoi(input);
        }
        catch (...)
        {
            UI::printErrorMessage("Lựa chọn không hợp lệ.");
            UI::displayGameMenuPrompt();
            return ClientState::GAME_MENU;
        }

        switch (choice)
        {
        case 1: // Auto Match
        {
            AutoMatchRequestMessage msg;
            msg.username = session_.getUsername();
            
            if (!network_.sendPacket(msg.getType(), msg.serialize()))
            {
                UI::printErrorMessage("Gửi yêu cầu ghép trận thất bại.");
                UI::displayGameMenuPrompt();
                return ClientState::GAME_MENU;
            }
            
            UI::displayWaitingAutoMatch();
            return ClientState::WAITING_AUTO_MATCH;
        }
            
        case 2: // Player List
        {
            RequestPlayerListMessage msg;
            
            if (!network_.sendPacket(msg.getType(), msg.serialize()))
            {
                UI::printErrorMessage("Gửi yêu cầu danh sách thất bại.");
                UI::displayGameMenuPrompt();
                return ClientState::GAME_MENU;
            }
            
            UI::printInfoMessage("Đang tải danh sách người chơi...");
            return ClientState::WAITING_PLAYER_LIST;
        }
            
        case 3: // Back to initial menu
            UI::clearConsole();
            UI::printLogo();
            UI::displayInitialMenuPrompt();
            return ClientState::INITIAL_MENU;
            
        default:
            UI::printErrorMessage("Lựa chọn không hợp lệ.");
            UI::displayGameMenuPrompt();
            return ClientState::GAME_MENU;
        }
    }

    // ==================== Auto Match Decision ====================

    ClientState processAutoMatchDecision(const std::string &input, StateContext &context)
    {
        int choice = 0;
        try
        {
            choice = std::stoi(input);
        }
        catch (...)
        {
            UI::printErrorMessage("Vui lòng chọn 1 (Chấp nhận) hoặc 2 (Từ chối).");
            UI::displayAutoMatchOptionsPrompt(context.opponent_username, 
                                              context.opponent_elo, 
                                              context.pending_game_id);
            return ClientState::AUTO_MATCH_DECISION;
        }

        if (choice == 1) // Accept
        {
            AutoMatchAcceptedMessage msg;
            msg.game_id = context.pending_game_id;
            
            if (!network_.sendPacket(msg.getType(), msg.serialize()))
            {
                UI::printErrorMessage("Gửi phản hồi thất bại.");
                UI::displayGameMenuPrompt();
                return ClientState::GAME_MENU;
            }
            
            UI::printInfoMessage("Đã chấp nhận. Đang chờ đối thủ...");
            return ClientState::WAITING_MATCH_START;
        }
        else if (choice == 2) // Decline
        {
            AutoMatchDeclinedMessage msg;
            msg.game_id = context.pending_game_id;
            
            if (!network_.sendPacket(msg.getType(), msg.serialize()))
            {
                UI::printErrorMessage("Gửi phản hồi thất bại.");
            }
            
            UI::printInfoMessage("Đã từ chối trận đấu.");
            context.clear();
            UI::clearConsole();
            UI::displayGameMenuPrompt();
            return ClientState::GAME_MENU;
        }
        else
        {
            UI::printErrorMessage("Vui lòng chọn 1 (Chấp nhận) hoặc 2 (Từ chối).");
            UI::displayAutoMatchOptionsPrompt(context.opponent_username, 
                                              context.opponent_elo, 
                                              context.pending_game_id);
            return ClientState::AUTO_MATCH_DECISION;
        }
    }

    // ==================== Player List & Challenge ====================

    ClientState processPlayerListView(const std::string &input, StateContext &context)
    {
        int choice = 0;
        try
        {
            choice = std::stoi(input);
        }
        catch (...)
        {
            UI::printErrorMessage("Vui lòng chọn 1 (Thách đấu) hoặc 2 (Quay lại).");
            UI::displayPlayerList(context.player_list_cache, session_.getUsername());
            return ClientState::PLAYER_LIST_VIEW;
        }

        if (choice == 1) // Challenge
        {
            UI::displayChallengeInputPrompt();
            return ClientState::CHALLENGE_INPUT;
        }
        else // Back
        {
            context.clear();
            UI::clearConsole();
            UI::displayGameMenuPrompt();
            return ClientState::GAME_MENU;
        }
    }

    ClientState processChallengeInput(const std::string &input, StateContext &context)
    {
        if (input.empty())
        {
            UI::printErrorMessage("Vui lòng nhập tên người chơi.");
            UI::displayChallengeInputPrompt();
            return ClientState::CHALLENGE_INPUT;
        }

        // Validate: can't challenge self
        if (input == session_.getUsername())
        {
            UI::printErrorMessage("Không thể thách đấu chính mình.");
            UI::displayGameMenuPrompt();
            return ClientState::GAME_MENU;
        }

        // Check if player is in list
        bool found = false;
        bool in_game = false;
        for (const auto &player : context.player_list_cache)
        {
            if (player.username == input)
            {
                found = true;
                in_game = player.in_game;
                break;
            }
        }

        if (!found)
        {
            UI::printErrorMessage("Người chơi không online hoặc không tồn tại.");
            UI::displayGameMenuPrompt();
            return ClientState::GAME_MENU;
        }

        if (in_game)
        {
            UI::printErrorMessage("Người chơi đang trong trận đấu.");
            UI::displayGameMenuPrompt();
            return ClientState::GAME_MENU;
        }

        ChallengeRequestMessage msg;
        msg.to_username = input;
        
        if (!network_.sendPacket(msg.getType(), msg.serialize()))
        {
            UI::printErrorMessage("Gửi thách đấu thất bại.");
            UI::displayGameMenuPrompt();
            return ClientState::GAME_MENU;
        }

        UI::displayWaitingChallengeResponse();
        return ClientState::WAITING_CHALLENGE_RESPONSE;
    }

    ClientState processChallengeReceived(const std::string &input, StateContext &context)
    {
        int choice = 0;
        try
        {
            choice = std::stoi(input);
        }
        catch (...)
        {
            UI::printErrorMessage("Vui lòng chọn 1 (Chấp nhận) hoặc 2 (Từ chối).");
            UI::displayChallengeDecisionPrompt(context.challenger_username, context.challenger_elo);
            return ClientState::CHALLENGE_RECEIVED;
        }

        ChallengeResponseMessage msg;
        msg.from_username = context.challenger_username;

        if (choice == 1) // Accept
        {
            msg.response = ChallengeResponseMessage::Response::ACCEPTED;
            
            if (!network_.sendPacket(msg.getType(), msg.serialize()))
            {
                UI::printErrorMessage("Gửi phản hồi thất bại.");
                UI::displayGameMenuPrompt();
                return ClientState::GAME_MENU;
            }
            
            UI::printInfoMessage("Đã chấp nhận thách đấu. Đang bắt đầu trận...");
            context.clear();
            return ClientState::WAITING_MATCH_START;
        }
        else // Decline
        {
            msg.response = ChallengeResponseMessage::Response::DECLINED;
            network_.sendPacket(msg.getType(), msg.serialize());
            
            UI::printInfoMessage("Đã từ chối thách đấu.");
            context.clear();
            UI::clearConsole();
            UI::displayGameMenuPrompt();
            return ClientState::GAME_MENU;
        }
    }

    // ==================== In-Game ====================

    ClientState processGameMove(const std::string &input)
    {
        if (input.empty())
        {
            UI::displayMovePrompt();
            return ClientState::IN_GAME_MY_TURN;
        }

        // Surrender
        if (input == "surrender" || input == "gg" || input == "quit")
        {
            SurrenderMessage msg;
            msg.game_id = session_.getGameId();
            msg.from_username = session_.getUsername();
            
            if (!network_.sendPacket(msg.getType(), msg.serialize()))
            {
                UI::printErrorMessage("Gửi lệnh đầu hàng thất bại.");
                UI::displayMovePrompt();
                return ClientState::IN_GAME_MY_TURN;
            }
            
            UI::printInfoMessage("Bạn đã đầu hàng!");
            // GAME_END message will follow
            return ClientState::IN_GAME_OPPONENT_TURN;
        }

        // Validate move format (basic check)
        if (input.length() < 4 || input.length() > 5)
        {
            UI::printErrorMessage("Định dạng nước đi không hợp lệ. VD: e2e4, a7a8q");
            UI::displayMovePrompt();
            return ClientState::IN_GAME_MY_TURN;
        }

        MoveMessage msg;
        msg.game_id = session_.getGameId();
        msg.uci_move = input;
        
        if (!network_.sendPacket(msg.getType(), msg.serialize()))
        {
            UI::printErrorMessage("Gửi nước đi thất bại.");
            UI::displayMovePrompt();
            return ClientState::IN_GAME_MY_TURN;
        }

        // Wait for server to validate and send status update
        return ClientState::IN_GAME_OPPONENT_TURN;
    }

    // ==================== Helper ====================

    void redisplayPrompt(ClientState state, StateContext &context)
    {
        switch (state)
        {
        case ClientState::INITIAL_MENU:
            UI::displayInitialMenuPrompt();
            break;
        case ClientState::WAITING_REGISTER_INPUT:
            UI::displayRegisterPrompt();
            break;
        case ClientState::WAITING_LOGIN_INPUT:
            UI::displayLoginPrompt();
            break;
        case ClientState::GAME_MENU:
            UI::displayGameMenuPrompt();
            break;
        case ClientState::AUTO_MATCH_DECISION:
            UI::displayAutoMatchOptionsPrompt(context.opponent_username, 
                                              context.opponent_elo, 
                                              context.pending_game_id);
            break;
        case ClientState::PLAYER_LIST_VIEW:
            UI::displayPlayerList(context.player_list_cache, session_.getUsername());
            break;
        case ClientState::CHALLENGE_INPUT:
            UI::displayChallengeInputPrompt();
            break;
        case ClientState::CHALLENGE_RECEIVED:
            UI::displayChallengeDecisionPrompt(context.challenger_username, context.challenger_elo);
            break;
        case ClientState::IN_GAME_MY_TURN:
            UI::displayMovePrompt();
            break;
        default:
            break;
        }
    }
};

#endif // INPUT_PROCESSOR_HPP

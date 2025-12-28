#ifndef CLIENT_STATE_HPP
#define CLIENT_STATE_HPP

#include <string>
#include <vector>
#include "../common/message.hpp"

/**
 * @brief Trạng thái của client trong state machine
 * 
 * Client chuyển đổi giữa các trạng thái dựa trên:
 * - Input từ user (stdin)
 * - Message từ server (socket)
 */
enum class ClientState {
    // Initial states
    INITIAL_MENU,               // Hiển thị menu đăng ký/đăng nhập/thoát
    WAITING_REGISTER_INPUT,     // Chờ user nhập username để đăng ký
    WAITING_LOGIN_INPUT,        // Chờ user nhập username để đăng nhập
    WAITING_REGISTER_RESPONSE,  // Đã gửi register, chờ server response
    WAITING_LOGIN_RESPONSE,     // Đã gửi login, chờ server response
    
    // Main menu states
    GAME_MENU,                  // Menu chính sau login
    
    // Auto match states
    WAITING_AUTO_MATCH,         // Đã gửi auto match request, đang chờ tìm
    AUTO_MATCH_DECISION,        // Tìm thấy match, chờ user accept/decline
    WAITING_MATCH_START,        // Đã accept, chờ game start
    
    // Player list & challenge states
    WAITING_PLAYER_LIST,        // Đã request player list, chờ response
    PLAYER_LIST_VIEW,           // Đang xem player list
    CHALLENGE_INPUT,            // Chờ user nhập username để challenge
    WAITING_CHALLENGE_RESPONSE, // Đã gửi challenge, chờ opponent response
    CHALLENGE_RECEIVED,         // Nhận challenge từ người khác, chờ user accept/decline
    
    // In-game states
    IN_GAME_MY_TURN,            // Trong game, lượt của mình - chờ user nhập move
    IN_GAME_OPPONENT_TURN,      // Trong game, chờ opponent move
    
    // Exit state
    EXITING                     // Đang thoát
};

/**
 * @brief Context data lưu trữ thông tin tạm thời giữa các state.
 *
 * Context này **không dùng để lưu trữ lâu dài**, chỉ phục vụ việc
 * truyền và giữ trạng thái dữ liệu trong quá trình client hoạt động.
 *
 * @details
 * Bao gồm các nhóm dữ liệu sau:
 * - Auto match:
 *   - Pending game ID
 *   - Thông tin đối thủ
 * - Challenge:
 *   - Thông tin người thách đấu
 * - Cache danh sách người chơi
 * - Bộ đếm timeout cho các trạng thái chờ
 */
struct StateContext {
    // Auto match data
    std::string pending_game_id;
    std::string opponent_username;
    uint16_t opponent_elo;
    
    // Challenge data
    std::string challenger_username;
    uint16_t challenger_elo;
    
    // Player list cache
    std::vector<PlayerListMessage::Player> player_list_cache;
    
    // Timeout tracking
    int timeout_counter;
    
    void clear() {
        pending_game_id.clear();
        opponent_username.clear();
        opponent_elo = 0;
        challenger_username.clear();
        challenger_elo = 0;
        player_list_cache.clear();
        timeout_counter = 0;
    }
};

/**
 * @brief Helper function to get state name for debugging
 */
inline const char* getStateName(ClientState state) {
    switch (state) {
        case ClientState::INITIAL_MENU: return "INITIAL_MENU";
        case ClientState::WAITING_REGISTER_INPUT: return "WAITING_REGISTER_INPUT";
        case ClientState::WAITING_LOGIN_INPUT: return "WAITING_LOGIN_INPUT";
        case ClientState::WAITING_REGISTER_RESPONSE: return "WAITING_REGISTER_RESPONSE";
        case ClientState::WAITING_LOGIN_RESPONSE: return "WAITING_LOGIN_RESPONSE";
        case ClientState::GAME_MENU: return "GAME_MENU";
        case ClientState::WAITING_AUTO_MATCH: return "WAITING_AUTO_MATCH";
        case ClientState::AUTO_MATCH_DECISION: return "AUTO_MATCH_DECISION";
        case ClientState::WAITING_MATCH_START: return "WAITING_MATCH_START";
        case ClientState::WAITING_PLAYER_LIST: return "WAITING_PLAYER_LIST";
        case ClientState::PLAYER_LIST_VIEW: return "PLAYER_LIST_VIEW";
        case ClientState::CHALLENGE_INPUT: return "CHALLENGE_INPUT";
        case ClientState::WAITING_CHALLENGE_RESPONSE: return "WAITING_CHALLENGE_RESPONSE";
        case ClientState::CHALLENGE_RECEIVED: return "CHALLENGE_RECEIVED";
        case ClientState::IN_GAME_MY_TURN: return "IN_GAME_MY_TURN";
        case ClientState::IN_GAME_OPPONENT_TURN: return "IN_GAME_OPPONENT_TURN";
        case ClientState::EXITING: return "EXITING";
        default: return "UNKNOWN";
    }
}

#endif // CLIENT_STATE_HPP

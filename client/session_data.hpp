#ifndef SESSION_DATA_HPP
#define SESSION_DATA_HPP

#include <string>

/**
 * @brief Struct lưu trữ trạng thái game hiện tại
 */
struct GameStatus
{
    std::string game_id = "";
    bool is_my_turn = false;
    bool is_white = false;
    std::string fen = "";
};

/**
 * @brief Singleton quản lý dữ liệu phiên làm việc của người dùng.
 * 
 * Lưu trữ thông tin như tên người dùng, trạng thái trò chơi, điểm ELO.
 * Phiên bản đơn giản cho single-threaded architecture.
 */
class SessionData {
public:
    static SessionData& getInstance() {
        static SessionData instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    SessionData(const SessionData&) = delete;
    SessionData& operator=(const SessionData&) = delete;

    // Username
    const std::string& getUsername() const {
        return username_;
    }

    void setUsername(const std::string& username) {
        username_ = username;
    }

    // ELO
    uint16_t getElo() const {
        return elo_;
    }

    void setElo(uint16_t elo) {
        elo_ = elo;
    }

    // Game status
    const std::string& getGameId() const {
        return game_status_.game_id;
    }

    void setGameStatus(const std::string& game_id, bool is_white, const std::string& fen) {
        game_status_.game_id = game_id;
        game_status_.is_my_turn = is_white; // White starts first
        game_status_.is_white = is_white;
        game_status_.fen = fen;
    }

    void clearGameStatus() {
        game_status_.game_id = "";
        game_status_.is_my_turn = false;
        game_status_.is_white = false;
        game_status_.fen = "";
    }

    void setTurn(bool is_my_turn) {
        game_status_.is_my_turn = is_my_turn;
    }

    bool isMyTurn() const {
        return game_status_.is_my_turn;
    }

    bool isWhite() const {
        return game_status_.is_white;
    }

    const std::string& getFen() const {
        return game_status_.fen;
    }

    void setFen(const std::string& fen) {
        game_status_.fen = fen;
    }

    bool isInGame() const {
        return !game_status_.game_id.empty();
    }

private:
    SessionData() : username_(""), elo_(0) {}

    std::string username_;
    uint16_t elo_;
    GameStatus game_status_;
};

#endif // SESSION_DATA_HPP
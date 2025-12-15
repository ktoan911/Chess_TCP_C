#ifndef SESSION_DATA_HPP
#define SESSION_DATA_HPP

#include <string>
#include <mutex>
#include <atomic>

#include "input_handler.hpp"

struct GameStatus
{
    std::string game_id = "";
    bool is_my_turn;
    bool is_white;
    std::string fen = "";
};

/**
 * @brief Singleton quản lý dữ liệu phiên làm việc của người dùng.
 * 
 * Lưu trữ thông tin như tên người dùng, trạng thái trò chơi, điểm ELO, 
 * và kiểm soát trạng thái hoạt động. Đảm bảo truy cập dữ liệu an toàn trong môi trường đa luồng.
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

    bool isCurrentHandler() {
        return std::this_thread::get_id() == current_handler_id.load();
    }

    void setCurrentHandler(std::thread::id id) {
        std::lock_guard<std::mutex> lock(handler_mutex);
        current_handler_id.store(id);
        InputHandler::setCancelCheck([this]() {
            return !isCurrentHandler();
        });
    }

    bool shouldStop() {
        return !isCurrentHandler();
    }

    std::atomic<bool>& getRunningAtomic() {
        return running;
    }

    bool getRunning() const {
        return running.load();
    }

    void setRunning(bool value) {
        running.store(value);
    }

    // Getters and setters for username and game_id
    std::string getUsername() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return username_;
    }

    void setUsername(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        username_ = username;
    }

    std::string getGameId() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return game_status_.game_id;
    }

    void setGameStatus(std::string game_id, bool is_white, std::string fen) {
        std::lock_guard<std::mutex> lock(mutex_);
        game_status_.game_id = game_id;
        game_status_.is_my_turn = is_white;
        game_status_.is_white = is_white;
        game_status_.fen = fen;
    }

    void clearGameStatus() {
        std::lock_guard<std::mutex> lock(mutex_);
        game_status_.game_id = "";
        game_status_.is_my_turn = false;
        game_status_.is_white = false;
        game_status_.fen = "";
    }

    void setTurn(bool is_my_turn) {
        std::lock_guard<std::mutex> lock(mutex_);
        game_status_.is_my_turn = is_my_turn;
    }

    bool isMyTurn() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return game_status_.is_my_turn;
    }

    bool isWhite() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return game_status_.is_white;
    }

    std::string getFen() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return game_status_.fen;
    }

    void setFen(std::string fen) {
        std::lock_guard<std::mutex> lock(mutex_);
        game_status_.fen = fen;
    }

    bool isInGame() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !game_status_.game_id.empty();
    }

    uint16_t getElo() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return elo_;
    }

    void setElo(uint16_t elo) {
        std::lock_guard<std::mutex> lock(mutex_);
        elo_ = elo;
    }

private:
    SessionData() : username_(""), elo_(0) {}

    std::atomic<bool> running;

    std::string username_;
    uint16_t elo_;
    GameStatus game_status_;

    mutable std::mutex mutex_;

    std::atomic<std::thread::id> current_handler_id;
    std::mutex handler_mutex;
};

#endif // SESSION_DATA_HPP
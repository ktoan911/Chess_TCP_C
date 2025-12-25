#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>
#include <queue>
#include <condition_variable>
#include <thread>

#include "../chess_engine/chess.hpp"
#include "../common/const.hpp"
#include "../common/message.hpp"

#include "data_storage.hpp"
#include "network_server.hpp"

/**
 * @class Game
 * @brief Quản lý trạng thái và logic của một ván cờ.
 *
 * Lớp này bao gồm các thông tin về người chơi, trạng thái bàn cờ,
 * lượt chơi hiện tại, và các phương thức để thực hiện nước đi,
 * kiểm tra trạng thái kết thúc của trò chơi, và các thông tin liên quan khác.
 */
class Game
{
public:
    std::string game_id;
    std::string player_white_name;
    std::string player_black_name;
    std::string current_turn;

    std::string winner;

    Game(const std::string &id,
         const std::string &p1,
         const std::string &p2,
         const std::string &fen) : game_id(id),
                                   player_white_name(p1),
                                   player_black_name(p2),
                                   board(fen),
                                   is_over(false),
                                   winner("")
    {
        chess::Color current_turn_color = board.sideToMove();
        bool isWhiteTurn = current_turn_color == chess::Color::WHITE;
        current_turn = isWhiteTurn ? player_white_name : player_black_name;
    }

    bool makeMove(const std::string &uci_move)
    {
        chess::Move move = chess::uci::uciToMove(board, uci_move);
        if (!isValidMove(board, move))
            return false;

        board.makeMove(move);
        half_moves_count++;

        // Kiểm tra kết quả trò chơi
        std::tie(reason, result) = board.isGameOver();

        if (result == chess::GameResult::NONE)
        {
            // Chuyển lượt chơi
            toggleTurn();
        }
        else
        {
            is_over = true;
            if (result == chess::GameResult::DRAW)
                winner = "<0>";
            else if (result == chess::GameResult::LOSE)
                winner = (current_turn == player_white_name) ? player_white_name : player_black_name;
        }

        return true;
    }

    bool isInCheck()
    {
        // Get the king's square for the current turn
        chess::Color current_turn_color = (current_turn == player_white_name) ? chess::Color::WHITE : chess::Color::BLACK;
        chess::Square king = board.kingSq(current_turn_color);

        // Determine the opponent's color
        chess::Color opponent = (current_turn_color == chess::Color::WHITE) ? chess::Color::BLACK : chess::Color::WHITE;

        // Check if the king's square is attacked by the opponent
        return board.isAttacked(king, opponent);
    }

    bool isGameOver()
    {
        return is_over;
    }

    std::string getFen()
    {
        return board.getFen();
    }

    std::string getResult()
    {
        switch (result)
        {
        case chess::GameResult::LOSE:
            return winner + " wins";
        case chess::GameResult::DRAW:
            return "draw";
        default:
            return "";
        }
    }

    std::string getResultReason()
    {
        switch (reason)
        {
        case chess::GameResultReason::CHECKMATE:
            return "checkmate";
        case chess::GameResultReason::STALEMATE:
            return "stalemate";
        case chess::GameResultReason::INSUFFICIENT_MATERIAL:
            return "insufficient material";
        case chess::GameResultReason::FIFTY_MOVE_RULE:
            return "fifty move rule";
        case chess::GameResultReason::THREEFOLD_REPETITION:
            return "threefold repetition";
        default:
            return "";
        }
    }

    int getHalfMovesCount() const
    {
        return half_moves_count;
    }

private:
    bool is_over;

    chess::Board board;
    chess::GameResult result = chess::GameResult::NONE;
    chess::GameResultReason reason = chess::GameResultReason::NONE;
    int half_moves_count = 0;

    bool isValidMove(const chess::Board &board, const chess::Move &move)
    {
        if (move == chess::Move::NO_MOVE)
        {
            return false;
        }

        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);
        if (std::find(moves.begin(), moves.end(), move) == moves.end())
        {
            return false;
        }

        return true;
    }

    void toggleTurn()
    {
        current_turn = (current_turn == player_white_name) ? player_black_name : player_white_name;
    }
};

struct PendingGame
{
    std::string game_id;
    int player1_fd;
    int player2_fd;
    bool player1_accepted;
    bool player2_accepted;

    // Default constructor
    PendingGame()
        : game_id(""), player1_fd(-1), player2_fd(-1),
          player1_accepted(false), player2_accepted(false) {}

    // Parameterized constructor
    PendingGame(const std::string &id, int fd1, int fd2)
        : game_id(id), player1_fd(fd1), player2_fd(fd2),
          player1_accepted(false), player2_accepted(false) {}
};

/**
 * @class GameManager
 * @brief Quản lý các ván cờ và hệ thống ghép trận.
 *
 * Lớp này chịu trách nhiệm quản lý các ván cờ đang diễn ra,
 * xử lý các yêu cầu di chuyển, và ghép trận cho người chơi.
 * Sử dụng Singleton pattern để đảm bảo chỉ có một instance duy nhất.
 *
 * Các chức năng chính bao gồm:
 *
 * - Tạo và quản lý các ván cờ.
 *
 * - Xử lý các yêu cầu di chuyển từ người chơi.
 *
 * - Ghép trận tự động dựa trên ELO của người chơi.
 *
 * - Gửi thông báo về trạng thái trò chơi và kết quả trận đấu cho người chơi.
 *
 * - Quản lý hàng đợi ghép trận và xử lý các yêu cầu chấp nhận hoặc từ chối ghép trận.
 *
 */
class GameManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<Game>> games;
    std::unordered_map<std::string, PendingGame> pending_games;
    std::mutex games_mutex;



    std::queue<int> matchmaking_queue; // Queue of client_fds
    std::condition_variable cv;
    bool stop_matching;
    std::thread matchmaking_thread;

    std::mutex matchmaking_mutex;

    // Private constructor for Singleton
    GameManager() : stop_matching(false), matchmaking_thread(&GameManager::matchmakingLoop, this) {}

    /**
     * @brief Vòng lặp quản lý tìm kiếm trận đấu.
     *
     * Hàm này liên tục kiểm tra hàng đợi tìm kiếm để ghép cặp người chơi.
     *
     * - Chờ đợi đến khi có ít nhất 2 người chơi trong hàng đợi hoặc khi nhận được tín hiệu dừng.
     *
     * - Nếu nhận được tín hiệu dừng, dừng vòng lặp.
     *
     * - Khi có đủ hai người chơi, lấy hai người từ hàng đợi và kiểm tra sự khác biệt ELO.
     *
     *   - 1. Nếu sự khác biệt ELO nhỏ hơn hoặc bằng ngưỡng cho phép, tạo trận đấu mới và gửi thông báo cho cả hai người chơi.
     *
     *   - 2. Nếu không, đưa lại hai người chơi vào hàng đợi.
     *
     * - Sleep 1 giây trước khi lặp lại vòng lặp.
     *
     * @note Hàm này chạy trong một luồng riêng và sử dụng mutex cùng điều kiện biến để quản lý truy cập vào hàng đợi tìm kiếm.
     */
    void matchmakingLoop()
    {
        int count = 0;
        while (true)
        {
            std::unique_lock<std::mutex> lock(matchmaking_mutex);
            cv.wait(lock, [this]
                    { return matchmaking_queue.size() >= 2 || stop_matching; });

            if (count % 10 == 0)
            {
                std::cout << "\nMatchmaking loop " << count++
                          << ", queue size: " << matchmaking_queue.size() << std::endl;
            }

            if (stop_matching)
            {
                std::cout << "Stopping matchmaking loop." << std::endl;
                break;
            }

            if (matchmaking_queue.size() >= 2)
            {
                int client1_fd = matchmaking_queue.front();
                matchmaking_queue.pop();
                int client2_fd = matchmaking_queue.front();
                matchmaking_queue.pop();

                NetworkServer &network_server = NetworkServer::getInstance();
                DataStorage &data_storage = DataStorage::getInstance();

                // Retrieve usernames and ELOs
                std::string username1 = network_server.getUsername(client1_fd);
                std::string username2 = network_server.getUsername(client2_fd);

                uint16_t elo1 = data_storage.getUserELO(username1);
                uint16_t elo2 = data_storage.getUserELO(username2);

                if (abs(static_cast<int>(elo1) - static_cast<int>(elo2)) <= Const::ELO_THRESHOLD)
                {
                    // Create new game
                    std::string game_id = createGame(username1, username2);

                    // Add to pending_games
                    pending_games[game_id] = PendingGame(game_id, client1_fd, client2_fd);

                    // Send AutoMatchFoundMessage to both clients
                    AutoMatchFoundMessage auto_match_found_msg_1;
                    auto_match_found_msg_1.opponent_username = username2;
                    auto_match_found_msg_1.opponent_elo = elo2;
                    auto_match_found_msg_1.game_id = game_id;
                    std::vector<uint8_t> serialized_1 = auto_match_found_msg_1.serialize();
                    network_server.sendPacket(client1_fd, MessageType::AUTO_MATCH_FOUND, serialized_1);

                    AutoMatchFoundMessage auto_match_found_msg_2;
                    auto_match_found_msg_2.opponent_username = username1;
                    auto_match_found_msg_2.opponent_elo = elo1;
                    auto_match_found_msg_2.game_id = game_id;
                    std::vector<uint8_t> serialized_2 = auto_match_found_msg_2.serialize();
                    network_server.sendPacket(client2_fd, MessageType::AUTO_MATCH_FOUND, serialized_2);
                }
                else
                {
                    // ELO difference too high, re-add to queue
                    matchmaking_queue.push(client1_fd);
                    matchmaking_queue.push(client2_fd);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    bool makeMove(const std::string &game_id, const std::string &uci_move)
    {
        auto game = getGame(game_id);
        if (game && !game->isGameOver())
            return game->makeMove(uci_move);
        return false;
    }

    std::shared_ptr<Game> getGameByClientFd(int client_fd)
    {
        std::string username = NetworkServer::getInstance().getUsername(client_fd);

        for (const auto &game_pair : games)
        {
            std::shared_ptr<Game> game = game_pair.second;
            if (game->player_white_name == username || game->player_black_name == username)
            {
                return game;
            }
        }
        return nullptr;
    }

public:
    // Delete copy constructor and assignment operator
    GameManager(const GameManager &) = delete;
    GameManager &operator=(const GameManager &) = delete;

    static GameManager &getInstance()
    {
        static GameManager instance;
        return instance;
    }

    /**
     * Tạo trận đấu mới với tên người chơi trắng và đen, và chuỗi FEN ban đầu.
     *
     * @param player_white_name Tên người chơi trắng.
     * @param player_black_name Tên người chơi đen.
     * @param initial_fen State ban đầu của ván cờ (mặc định: STARTPOS).
     * @return Mã định danh của trận đấu được tạo.
     */
    std::string createGame(const std::string &player_white_name, const std::string &player_black_name, const std::string &initial_fen = chess::constants::STARTPOS)
    {
        std::lock_guard<std::mutex> lock(games_mutex);

        // Generate game_id based on player names and current time with higher precision
        using namespace std::chrono;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        auto in_time_t = system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&in_time_t);
        std::ostringstream oss;
        char buffer[30];
        std::strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &tm);
        oss << "game_" << player_white_name << "_" << player_black_name << "_" << buffer << "_" << ms.count();
        std::string game_id = oss.str();

        games[game_id] = std::make_shared<Game>(game_id, player_white_name, player_black_name, initial_fen);

        // Get player IPs
        NetworkServer &network_server = NetworkServer::getInstance();
        std::string white_ip = network_server.getClientIPByUsername(player_white_name);
        std::string black_ip = network_server.getClientIPByUsername(player_black_name);

        DataStorage &datastorage = DataStorage::getInstance();
        datastorage.registerMatch(game_id, player_white_name, player_black_name, initial_fen, white_ip, black_ip);

        return game_id;
    }


    std::shared_ptr<Game> getGame(const std::string &game_id)
    {
        std::lock_guard<std::mutex> lock(games_mutex);

        auto it = games.find(game_id);
        if (it != games.end())
            return it->second;
        return nullptr;
    }

    std::vector<std::shared_ptr<Game>> getAllGames()
    {
        std::lock_guard<std::mutex> lock(games_mutex);
        std::vector<std::shared_ptr<Game>> allGames;
        for (const auto &pair : games)
        {
            allGames.push_back(pair.second);
        }
        return allGames;
    }

    bool removeGame(const std::string &id)
    {
        std::lock_guard<std::mutex> lock(games_mutex);

        return games.erase(id) > 0;
    }

    /**
     * @brief Xử lý nước đi của người chơi trong trò chơi cờ vua.
     *
     *  - Kiểm tra tính hợp lệ của nước đi.
     *
     *  - Cập nhật trạng thái trò chơi và lưu vào cơ sở dữ liệu.
     *
     *  - Thông báo cho người chơi.
     *
     *  - Kiểm tra xem trò chơi đã kết thúc chưa.
     *
     * @param client_fd ID kết nối của khách hàng.
     * @param game_id ID của trò chơi.
     * @param uci_move Nước đi theo định dạng UCI.
     */
    void handleMove(int client_fd, const std::string &game_id, const std::string &uci_move)
    {
        if (makeMove(game_id, uci_move))
        {
            // Retrieve game information
            std::shared_ptr<Game> game = getGame(game_id);

            // Save the player's move to the database
            DataStorage &data_storage = DataStorage::getInstance();
            data_storage.addMove(game_id, uci_move, getGameFen(game_id));

            // Notify players about the move
            notifyPlayers(game_id, game);

            // Check if the game is over
            bool is_game_over = isGameOver(game_id);

            if (is_game_over)
            {
                endGame(game_id, game);
                return;
            }
        }
        else
        {
            // Invalid move
            NetworkServer &network_server = NetworkServer::getInstance();
            InvalidMoveMessage invalid_move_msg;
            invalid_move_msg.game_id = game_id;
            invalid_move_msg.error_message = "Invalid move: " + uci_move;

            std::vector<uint8_t> serialized = invalid_move_msg.serialize();
            network_server.sendPacket(client_fd, MessageType::INVALID_MOVE, serialized);
        }
    }

    void notifyPlayers(const std::string &game_id, const std::shared_ptr<Game> &game)
    {
        std::string player_white_name = game->player_white_name;
        std::string player_black_name = game->player_black_name;

        NetworkServer &network_server = NetworkServer::getInstance();

        // Prepare GameStatusUpdateMessage
        GameStatusUpdateMessage game_status_update_msg;
        game_status_update_msg.game_id = game_id;
        game_status_update_msg.fen = getGameFen(game_id);
        game_status_update_msg.current_turn_username = getGameCurrentTurn(game_id);
        game_status_update_msg.is_game_over = isGameOver(game_id);

        if (game->isInCheck())
        {
            game_status_update_msg.message = "Check!";
        }
        else
        {
            game_status_update_msg.message = "";
        }

        // Serialize and send the update to both players
        std::vector<uint8_t> serialized = game_status_update_msg.serialize();
        network_server.sendPacketToUsername(player_white_name, MessageType::GAME_STATUS_UPDATE, serialized);
        network_server.sendPacketToUsername(player_black_name, MessageType::GAME_STATUS_UPDATE, serialized);
    }

    /**
     * Kết thúc trò chơi, cập nhật kết quả và thông báo cho người chơi.
     *
     * @param game_id ID của trò chơi.
     * @param game Con trỏ thông minh tới đối tượng trò chơi.
     */
    void endGame(const std::string &game_id, const std::shared_ptr<Game> &game)
    {
        std::string player_white_name = game->player_white_name;
        std::string player_black_name = game->player_black_name;

        // Wait briefly before ending the game
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Determine the winner and reason
        std::string winner = getGameWinner(game_id);
        std::string reason = getGameResultReason(game_id);
        uint16_t half_moves_count = getGameHalfMovesCount(game_id);

        DataStorage &data_storage = DataStorage::getInstance();
        data_storage.updateMatchResult(game_id, winner, reason);

        // Prepare and send GameEndMessage to both players
        GameEndMessage game_end_msg;
        game_end_msg.game_id = game_id;
        game_end_msg.winner_username = winner;
        game_end_msg.reason = reason;
        game_end_msg.half_moves_count = half_moves_count;

        std::vector<uint8_t> serialized_end = game_end_msg.serialize();
        NetworkServer &network_server = NetworkServer::getInstance();
        network_server.sendPacketToUsername(player_white_name, MessageType::GAME_END, serialized_end);
        network_server.sendPacketToUsername(player_black_name, MessageType::GAME_END, serialized_end);

        // Prepare and send GameLogMessage to both players
        try
        {
            MatchModel match = data_storage.getMatch(game_id);
            
            GameLogMessage game_log_msg;
            game_log_msg.game_id = game_id;
            game_log_msg.start_time = match.start_time.time_since_epoch().count();
            game_log_msg.end_time = match.end_time.time_since_epoch().count();
            game_log_msg.white_ip = match.white_ip;
            game_log_msg.black_ip = match.black_ip;
            game_log_msg.winner = winner;
            game_log_msg.reason = reason;
            
            // Collect all moves
            for (const auto &move : match.moves)
            {
                game_log_msg.moves.push_back(move.uci_move);
            }
            
            std::vector<uint8_t> serialized_log = game_log_msg.serialize();
            network_server.sendPacketToUsername(player_white_name, MessageType::GAME_LOG, serialized_log);
            network_server.sendPacketToUsername(player_black_name, MessageType::GAME_LOG, serialized_log);
            
            std::cout << "[GAME_LOG] Sent game log for " << game_id << " to both players." << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[GAME_LOG] Failed to send game log: " << e.what() << std::endl;
        }

        // Remove the game from active games
        removeGame(game_id);
    }

    /**
     * Xử lý khi một client ngắt kết nối.
     * 
     * - Nếu client đó đang chơi trò chơi, thông báo cho đối thủ và kết thúc trò chơi.
     * 
     * - Nếu client đó đang trong hàng đợi ghép trận, loại bỏ khỏi hàng đợi.
     * 
     *
     * @param client_fd File descriptor của client.
     */
    void clientDisconnected(int client_fd)
    {
        NetworkServer &network_server = NetworkServer::getInstance();
        std::string username = network_server.getUsername(client_fd);
        std::shared_ptr<Game> game = getGameByClientFd(client_fd);

        if (game != nullptr)
        {
            std::string game_id = game->game_id;
            std::string opponent_name;

            if (game->player_white_name == username)
            {
                opponent_name = game->player_black_name;
            }
            else if (game->player_black_name == username)
            {
                opponent_name = game->player_white_name;
            }

            // Send GameResultMessage to the opponent
            GameEndMessage game_end_msg;
            game_end_msg.game_id = game_id;
            game_end_msg.winner_username = opponent_name;
            game_end_msg.reason = "Opponent disconnected";
            game_end_msg.half_moves_count = game->getHalfMovesCount();
            network_server.sendPacketToUsername(opponent_name, MessageType::GAME_END, game_end_msg.serialize());

            // Update points (disconnect = lose: -3, opponent wins: +3)
            DataStorage &data_storage = DataStorage::getInstance();
            int current_elo = data_storage.getUserELO(username);
            int current_opponent_elo = data_storage.getUserELO(opponent_name);
            int new_elo = (current_elo >= 3) ? current_elo - 3 : 0;  // Lose -3
            int new_opponent_elo = current_opponent_elo + 3;  // Win +3

            data_storage.updateUserELO(username, new_elo);
            data_storage.updateUserELO(opponent_name, new_opponent_elo);

            // Remove the game from the system
            removeGame(game_id);
        }

        // Remove the client from the matchmaking queue
        removePlayerFromQueue(client_fd);
    }

    bool isGameOver(const std::string &game_id)
    {
        auto game = getGame(game_id);
        if (game)
            return game->isGameOver();
        return false;
    }

    std::string getGameFen(const std::string &game_id)
    {
        auto game = getGame(game_id);
        if (game)
            return game->getFen();
        return "";
    }

    std::string getGameCurrentTurn(const std::string &game_id)
    {
        auto game = getGame(game_id);
        if (game)
            return game->current_turn;
        return "";
    }

    std::string getGameWinner(const std::string &game_id)
    {
        auto game = getGame(game_id);
        if (game)
            return game->winner;
        return "";
    }

    std::string getGameResultReason(const std::string &game_id)
    {
        auto game = getGame(game_id);
        if (game)
            return game->getResultReason();
        return "";
    }

    uint16_t getGameHalfMovesCount(const std::string &game_id)
    {
        auto game = getGame(game_id);
        if (game)
            return game->getHalfMovesCount();
        return 0;
    }

    void addPlayerToQueue(int client_fd)
    {
        {
            std::lock_guard<std::mutex> lock(matchmaking_mutex);
            matchmaking_queue.push(client_fd);
        }
        cv.notify_one();
    }

    void removePlayerFromQueue(int client_fd)
    {
        std::lock_guard<std::mutex> lock(matchmaking_mutex);
        std::queue<int> new_queue;
        while (!matchmaking_queue.empty())
        {
            int front = matchmaking_queue.front();
            matchmaking_queue.pop();
            if (front != client_fd)
            {
                new_queue.push(front);
            }
        }
        matchmaking_queue = new_queue;
    }

    /**
     * Xử lý sự chấp nhận trận đấu từ client cho trò chơi được xác định bởi game_id.
     *
     * @param client_fd Định danh của khách hàng.
     * @param game_id ID của trò chơi đang chờ.
     */
    void handleAutoMatchAccepted(int client_fd, const std::string &game_id)
    {
        std::lock_guard<std::mutex> lock(games_mutex);
        auto it = pending_games.find(game_id);
        if (it != pending_games.end())
        {
            PendingGame &pending = it->second;
            if (client_fd == pending.player1_fd)
                pending.player1_accepted = true;
            else if (client_fd == pending.player2_fd)
                pending.player2_accepted = true;

            if (pending.player1_accepted && pending.player2_accepted)
            {
                // Both players accepted, game starts

                NetworkServer &network_server = NetworkServer::getInstance();

                // Notify both players about the game start
                GameStartMessage game_start_msg;
                game_start_msg.game_id = game_id;
                game_start_msg.player1_username = network_server.getUsername(pending.player1_fd);
                game_start_msg.player2_username = network_server.getUsername(pending.player2_fd);
                game_start_msg.starting_player_username = game_start_msg.player1_username; // Player 1 starts
                game_start_msg.fen = chess::constants::STARTPOS;

                std::vector<uint8_t> serialized = game_start_msg.serialize();

                network_server.sendPacket(pending.player1_fd, MessageType::GAME_START, serialized);
                network_server.sendPacket(pending.player2_fd, MessageType::GAME_START, serialized);

                // Remove from pending_games
                pending_games.erase(it);
            }
        }
    }

    /**
     * @brief Xử lý từ chối ghép trận tự động của người chơi.
     *
     * @param client_fd Mã định danh kết nối của khách hàng đã từ chối.
     * @param game_id ID của trận đấu bị từ chối.
     */
    void handleAutoMatchDeclined(int client_fd, const std::string &game_id)
    {
        std::lock_guard<std::mutex> lock(games_mutex);
        auto it = pending_games.find(game_id);
        if (it != pending_games.end())
        {
            PendingGame pending = it->second;
            pending_games.erase(it);

            NetworkServer &network_server = NetworkServer::getInstance();

            // Notify the other player about the declination
            int other_fd = (client_fd == pending.player1_fd) ? pending.player2_fd : pending.player1_fd;
            MatchDeclinedNotificationMessage decline_msg;
            decline_msg.game_id = game_id;
            std::vector<uint8_t> serialized = decline_msg.serialize();
            network_server.sendPacket(other_fd, decline_msg.getType(), serialized);

            // Requeue the other player
            matchmaking_queue.push(other_fd);
            cv.notify_one();
        }
    }

    bool isUserInGame(const std::string &username)
    {
        std::lock_guard<std::mutex> lock(games_mutex);
        for (const auto &game_pair : games)
        {
            std::shared_ptr<Game> game = game_pair.second;
            if (game->player_white_name == username ||
                game->player_black_name == username)
            {
                return true;
            }
        }
        return false;
    }

    std::string getUserGameId(const std::string &username)
    {
        if (!isUserInGame(username))
        {
            return "";
        }

        std::lock_guard<std::mutex> lock(games_mutex);
        for (const auto &game_pair : games)
        {
            std::shared_ptr<Game> game = game_pair.second;
            if (game->player_white_name == username ||
                game->player_black_name == username)
            {
                return game_pair.first;
            }
        }
        return "";
    }


    std::string getOpponent(const std::string &game_id, const std::string &player)
    {
        auto game = games.find(game_id);
        if (game == games.end())
            return "";

        if (game->second->player_white_name == player)
            return game->second->player_black_name;
        else if (game->second->player_black_name == player)
            return game->second->player_white_name;

        return "";
    }

    void endGameForSurrender(const std::string &game_id, const std::string &surrendering_player)
    {
        DataStorage &datastorage = DataStorage::getInstance();
        Game *game = getGame(game_id).get();

        std::string player_white_name = game->player_white_name;
        std::string player_black_name = game->player_black_name;

        std::string winner = (surrendering_player == player_white_name) ? player_black_name : player_white_name;
        std::string reason = "Player surrendered";

        // Gọi hàm updateMatchResult để cập nhật kết quả trận đấu
        datastorage.updateMatchResult(game_id, winner, reason);

        uint16_t white_elo = datastorage.getUserELO(player_white_name);
        uint16_t black_elo = datastorage.getUserELO(player_black_name);
        uint16_t new_white_elo = white_elo;
        uint16_t new_black_elo = black_elo;

        // Remove game
        removeGame(game_id);
    }
};

#endif // GAME_MANAGER_HPP
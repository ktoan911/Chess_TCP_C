// GAME_MANAGER_HPP - Quản lý trận đấu cờ vua, matchmaking, ELO

#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

// Thư viện chuẩn C++
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

// Thư viện dự án
#include "../chess_engine/chess.hpp"
#include "../common/message.hpp"
#include "data_storage.hpp"
#include "game_status.hpp"
#include "network_server.hpp"
#include "structs.hpp"

// Lớp GameManager - Singleton quản lý trận đấu, matchmaking
class GameManager {
private:
  std::string generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);  // 0-15 cho hex
    std::uniform_int_distribution<> dis2(8, 11); // variant byte
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++)
      ss << dis(gen);
    ss << "-";
    for (i = 0; i < 4; i++)
      ss << dis(gen);
    ss << "-4";
    for (i = 0; i < 3; i++)
      ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++)
      ss << dis(gen);
    ss << "-";
    for (i = 0; i < 12; i++)
      ss << dis(gen);
    return ss.str();
  }

  // Map lưu games: game_id -> shared_ptr<GameStatus>
  std::unordered_map<std::string, std::shared_ptr<GameStatus>> games;

  // Map lưu pending games chờ accep (dùng cho auto matchmaking)
  std::unordered_map<std::string, PendingGame> pending_games;

  std::mutex games_mutex; // Bảo vệ games và pending_games

  NetworkServer *network_server_; // Inject qua init()
  DataStorage *data_storage_;     // Inject qua init()
  bool initialized_;              // Cờ đánh dấu đã init

  // Matchmaking variables
  std::queue<int> matchmaking_queue; // Queue chứa client_fd đang chờ tìm trận
  std::condition_variable cv;        // Đóng bộ matchmaking thread
  bool stop_matching;                // Cờ dừng matchmaking loop
  std::thread matchmaking_thread;    // Thread chạy matchmaking loop
  std::mutex matchmaking_mutex;      // Bảo vệ matchmaking_queue

  // Constructor private (Singleton)
  GameManager()
      : network_server_(nullptr), data_storage_(nullptr), initialized_(false),
        stop_matching(false) {}

  // Vòng lặp matchmaking: chờ đủ 2 người, ghép cặp theo ELO
  void matchmakingLoop() {
    int count = 0;
    while (true) {
      std::unique_lock<std::mutex> lock(matchmaking_mutex);

      // Chờ đến khi có >= 2 người hoặc nhận tín hiệu dừng
      cv.wait(lock, [this] {
        return matchmaking_queue.size() >= 2 || stop_matching;
      });

      count++;
      if (count % 10 == 0) {
        std::cout << "\nMatchmaking loop " << count
                  << ", queue size: " << matchmaking_queue.size() << std::endl;
      }

      if (stop_matching) {
        std::cout << "Stopping matchmaking loop." << std::endl;
        break;
      }

      if (matchmaking_queue.size() >= 2) {
        // Lấy người chơi đầu tiên
        int client1_fd = matchmaking_queue.front();
        matchmaking_queue.pop();

        if (!network_server_->isClientConnected(client1_fd)) {
          lock.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }

        std::string username1 = network_server_->getUsername(client1_fd);
        uint16_t elo1 = data_storage_->getUserELO(username1);
        int rank1 = data_storage_->getUserRank(username1);

        // Biến lưu kết quả tìm kiếm
        int matched_client_fd = -1; // -1 = chưa tìm thấy
        std::string matched_username;
        uint16_t matched_elo = 0;

        // Lưu kích thước queue hiện tại (để duyệt đúng số lượng)
        size_t queue_size = matchmaking_queue.size();

        std::queue<int> temp_queue;

        // Duyệt qua tất cả các client trong queue
        for (size_t i = 0; i < queue_size; i++) {
          int candidate_fd = matchmaking_queue.front();
          matchmaking_queue.pop();

          // Kiểm tra candidate còn kết nối không
          if (!network_server_->isClientConnected(candidate_fd)) {
            // Ngắt kết nối → bỏ qua (không đưa vào temp_queue)
            continue;
          }

          // Nếu chưa tìm thấy match
          if (matched_client_fd == -1) {
            std::string candidate_username =
                network_server_->getUsername(candidate_fd);
            int candidate_rank = data_storage_->getUserRank(candidate_username);

            // Kiểm tra rank (bậc hạng) có phù hợp không
            // abs(rank1 - rank2) <= 10 (chênh không quá 10 bậc)
            if (abs(static_cast<int>(rank1) -
                    static_cast<int>(candidate_rank)) <= 10) {
              // Tìm thấy đối thủ phù hợp!
              matched_client_fd = candidate_fd;
              matched_username = candidate_username;
              matched_elo = data_storage_->getUserELO(candidate_username);
            } else {
              // Rank không khớp → giữ lại trong temp_queue
              temp_queue.push(candidate_fd);
            }
          } else {
            temp_queue.push(candidate_fd);
          }
        }

        // Đưa các client chưa match lại vào matchmaking_queue
        while (!temp_queue.empty()) {
          matchmaking_queue.push(temp_queue.front());
          temp_queue.pop();
        }

        if (matched_client_fd != -1) {
          // Tìm thấy đối thủ → Tạo game và gửi thông báo

          // Tạo game mới (sẽ được lưu vào database)
          std::string game_id = createGame(username1, matched_username);

          // Thêm vào pending_games (chờ cả hai chấp nhận)
          {
            std::lock_guard<std::mutex> games_lock(games_mutex);
            pending_games[game_id] =
                PendingGame(game_id, client1_fd, matched_client_fd);
          }

          // Unlock matchmaking_mutex trước khi gửi packet
          lock.unlock();

          // Gửi AUTO_MATCH_FOUND message cho client1
          AutoMatchFoundMessage auto_match_found_msg_1;
          auto_match_found_msg_1.opponent_username = matched_username;
          auto_match_found_msg_1.opponent_elo = matched_elo;
          auto_match_found_msg_1.game_id = game_id;
          std::vector<uint8_t> serialized_1 =
              auto_match_found_msg_1.serialize();
          network_server_->sendPacket(client1_fd, MessageType::AUTO_MATCH_FOUND,
                                      serialized_1);

          // Gửi AUTO_MATCH_FOUND message cho client2 (matched_client)
          AutoMatchFoundMessage auto_match_found_msg_2;
          auto_match_found_msg_2.opponent_username = username1;
          auto_match_found_msg_2.opponent_elo = elo1;
          auto_match_found_msg_2.game_id = game_id;
          std::vector<uint8_t> serialized_2 =
              auto_match_found_msg_2.serialize();
          network_server_->sendPacket(
              matched_client_fd, MessageType::AUTO_MATCH_FOUND, serialized_2);
        } else {
          // Không tìm thấy đối thủ → Đưa client1 vào cuối queue

          matchmaking_queue.push(client1_fd);
          lock.unlock();
        }
      } else {
        // Không đủ 2 người (không nên xảy ra do cv.wait)
        lock.unlock();
      }

      // Ngủ 1 giây để tránh CPU chạy quá nhanh (busy loop)
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  }

  bool makeMove(const std::string &game_id, const std::string &uci_move) {
    auto game = getGame(game_id);

    // Kiểm tra game tồn tại VÀ chưa kết thúc
    if (game && !game->isGameOver())
      return game->makeMove(uci_move);

    return false; // Game không tồn tại hoặc đã kết thúc
  }

  std::shared_ptr<GameStatus> getGameByClientFd(int client_fd) {
    // Lấy username của client
    std::string username = network_server_->getUsername(client_fd);

    // Duyệt qua tất cả các game đang diễn ra
    for (const auto &game_pair : games) {
      std::shared_ptr<GameStatus> game = game_pair.second;

      // Kiểm tra username có khớp với player nào không
      if (game->player_white_name == username ||
          game->player_black_name == username) {
        return game; // Tìm thấy!
      }
    }

    return nullptr; // Không tìm thấy game nào
  }

public:
  // Delete copy constructor - Ngăn sao chép instance
  GameManager(const GameManager &) = delete;

  // Delete assignment operator - Ngăn gán instance
  GameManager &operator=(const GameManager &) = delete;

  ~GameManager() {
    {
      // Lock mutex để set stop_matching an toàn
      std::lock_guard<std::mutex> lock(matchmaking_mutex);
      stop_matching = true;
    }

    // Đánh thức matchmaking thread (nếu đang chờ trong cv.wait)
    cv.notify_one();

    // Chờ matchmaking thread kết thúc
    if (matchmaking_thread.joinable()) {
      matchmaking_thread.join();
    }
  }

  static GameManager &getInstance() {
    static GameManager instance; // Tạo instance duy nhất
    return instance;
  }

  void init(NetworkServer &network_server, DataStorage &data_storage) {
    // Chỉ khởi tạo nếu chưa được khởi tạo
    if (!initialized_) {
      // Lưu con trỏ đến NetworkServer
      // & lấy địa chỉ của object network_server
      network_server_ = &network_server;

      // Lưu con trỏ đến DataStorage
      data_storage_ = &data_storage;

      // Đánh dấu đã khởi tạo xong
      initialized_ = true;

      // Tạo và khởi động matchmaking thread
      // &GameManager::matchmakingLoop: Con trỏ đến hàm thành viên
      // this: Con trỏ đến object GameManager hiện tại
      matchmaking_thread = std::thread(&GameManager::matchmakingLoop, this);
    }
  }

  std::string
  createGame(const std::string &player_white_name,
             const std::string &player_black_name,
             const std::string &initial_fen = chess::constants::STARTPOS) {
    // Lock để đảm bảo chỉ 1 thread tạo game tại một thời điểm
    std::lock_guard<std::mutex> lock(games_mutex);

    // Tạo UUID ngẫu nhiên làm game_id duy nhất
    std::string game_id = generateUUID();

    // Tạo GameStatus object mới và thêm vào map
    // make_shared: Tạo shared_ptr, tự động quản lý bộ nhớ
    games[game_id] = std::make_shared<GameStatus>(
        game_id, player_white_name, player_black_name, initial_fen);

    // Lấy địa chỉ IP của người chơi trắng
    std::string white_ip =
        network_server_->getClientIPByUsername(player_white_name);

    // Lấy địa chỉ IP của người chơi đen
    std::string black_ip =
        network_server_->getClientIPByUsername(player_black_name);

    // Lưu thông tin trận đấu vào database
    // Bao gồm: game_id, tên 2 người chơi, FEN, IP
    data_storage_->registerMatch(game_id, player_white_name, player_black_name,
                                 initial_fen, white_ip, black_ip);

    // Trả về game_id để caller có thể sử dụng
    return game_id;
  }

  std::shared_ptr<GameStatus> getGame(const std::string &game_id) {
    std::lock_guard<std::mutex> lock(games_mutex);

    // Tìm game trong map
    auto it = games.find(game_id);

    // Nếu tìm thấy (iterator != end)
    if (it != games.end())
      return it->second; // Trả về shared_ptr

    return nullptr; // Không tìm thấy
  }

  // Hàm này dùng để:
  std::vector<std::shared_ptr<GameStatus>> getAllGames() {
    std::lock_guard<std::mutex> lock(games_mutex);

    // Vector để chứa kết quả
    std::vector<std::shared_ptr<GameStatus>> allGames;

    // Duyệt qua tất cả các game trong map
    for (const auto &pair : games) {
      allGames.push_back(pair.second); // Thêm vào vector
    }

    return allGames;
  }

  // Khi nào gọi:
  bool removeGame(const std::string &id) {
    std::lock_guard<std::mutex> lock(games_mutex);
    // erase() trả về số lượng phần tử bị xóa
    // > 0 = xóa thành công, 0 = không tìm thấy
    return games.erase(id) > 0;
  }

  // Hàm này là hàm TRUNG TÂM xử lý mọi nước đi từ client.
  void handleMove(int client_fd, const std::string &game_id,
                  const std::string &uci_move) {
    // Thử thực hiện nước đi
    if (makeMove(game_id, uci_move)) {
      // NƯỚC ĐI HỢP LỆ - Cập nhật và thông báo

      // Lấy thông tin game (để notify players)
      std::shared_ptr<GameStatus> game = getGame(game_id);

      // Lưu nước đi vào database
      // Lưu cả: UCI move + FEN state sau nước đi (để có thể replay)
      data_storage_->addMove(game_id, uci_move, getGameFen(game_id));

      // Gửi thông báo cập nhật cho CẢ HAI người chơi
      notifyPlayers(game_id, game);

      bool is_game_over = isGameOver(game_id);

      if (is_game_over) {
        // Game kết thúc → xử lý kết thúc (update ELO, send results)
        endGame(game_id, game);
        return; // Kết thúc hàm
      }
    } else {
      // NƯỚC ĐI KHÔNG HỢP LỆ - Gửi thông báo lỗi

      InvalidMoveMessage invalid_move_msg;
      invalid_move_msg.game_id = game_id;
      invalid_move_msg.error_message = "Invalid move: " + uci_move;

      // Serialize message thành bytes
      std::vector<uint8_t> serialized = invalid_move_msg.serialize();

      // Chỉ gửi cho người gửi nước đi sai (không gửi cho đối thủ)
      network_server_->sendPacket(client_fd, MessageType::INVALID_MOVE,
                                  serialized);
    }
  }

  // Hàm này được gọi SAU MỖI NƯỚC ĐI để đồng bộ trạng thái game
  void notifyPlayers(const std::string &game_id,
                     const std::shared_ptr<GameStatus> &game) {
    // Lấy tên 2 người chơi
    std::string player_white_name = game->player_white_name;
    std::string player_black_name = game->player_black_name;

    // Chuẩn bị message cập nhật trạng thái
    GameStatusUpdateMessage game_status_update_msg;
    game_status_update_msg.game_id = game_id;
    game_status_update_msg.fen = getGameFen(game_id); // Trạng thái bàn cờ
    game_status_update_msg.current_turn_username =
        getGameCurrentTurn(game_id);                           // Lượt
    game_status_update_msg.is_game_over = isGameOver(game_id); // Đã kết thúc?

    // Kiểm tra xem có bị chiếu (check) không
    if (game->isInCheck()) {
      game_status_update_msg.message = "Check!"; // Thông báo "Chiếu!"
    } else {
      game_status_update_msg.message = ""; // Không có message đặc biệt
    }

    // Serialize message thành bytes để gửi qua network
    std::vector<uint8_t> serialized = game_status_update_msg.serialize();

    // Gửi cho người chơi trắng
    network_server_->sendPacketToUsername(
        player_white_name, MessageType::GAME_STATUS_UPDATE, serialized);

    // Gửi cho người chơi đen (CÙNG message)
    network_server_->sendPacketToUsername(
        player_black_name, MessageType::GAME_STATUS_UPDATE, serialized);
  }

  void endGame(const std::string &game_id,
               const std::shared_ptr<GameStatus> &game) {
    // Lấy tên hai người chơi từ game object
    std::string player_white_name = game->player_white_name;
    std::string player_black_name = game->player_black_name;

    // Tại sao sleep?
    // - Cho client kịp nhận và hiển thị nước đi cuối cùng
    // - Tránh GameEnd message đến trước GameStatusUpdate message
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Lấy tên người thắng (hoặc "<0>" nếu hòa)
    std::string winner = getGameWinner(game_id);

    // Lấy lý do kết thúc (checkmate, stalemate, etc.)
    std::string reason = getGameResultReason(game_id);

    // Lấy số nước đi (half-moves)
    uint16_t half_moves_count = getGameHalfMovesCount(game_id);

    data_storage_->updateMatchResult(game_id, winner, reason);

    // Lấy ELO hiện tại của cả hai người chơi
    uint16_t white_elo = data_storage_->getUserELO(player_white_name);
    uint16_t black_elo = data_storage_->getUserELO(player_black_name);

    // Xét 3 trường hợp:
    if (winner == "<0>") {
      // CASE 1: HÒA (Draw)
      // Không làm gì cả, ELO giữ nguyên

    } else if (winner == player_white_name) {
      // CASE 2: TRẮNG THẮNG
      // Trắng: +3 điểm
      data_storage_->updateUserELO(player_white_name, white_elo + 3);

      // Đen: -3 điểm (nhưng không xuống dưới 0)
      data_storage_->updateUserELO(player_black_name,
                                   (black_elo >= 3) ? black_elo - 3 : 0);

    } else {
      // CASE 3: ĐEN THẮNG
      // Đen: +3 điểm
      data_storage_->updateUserELO(player_black_name, black_elo + 3);

      // Trắng: -3 điểm (không xuống dưới 0)
      data_storage_->updateUserELO(player_white_name,
                                   (white_elo >= 3) ? white_elo - 3 : 0);
    }

    // Chuẩn bị message
    GameEndMessage game_end_msg;
    game_end_msg.game_id = game_id;
    game_end_msg.winner_username = winner; // Người thắng (hoặc "<0>")
    game_end_msg.reason = reason;          // Lý do kết thúc
    game_end_msg.half_moves_count = half_moves_count; // Số nước đi

    // Serialize và gửi cho CẢ HAI người chơi
    std::vector<uint8_t> serialized_end = game_end_msg.serialize();
    network_server_->sendPacketToUsername(
        player_white_name, MessageType::GAME_END, serialized_end);
    network_server_->sendPacketToUsername(
        player_black_name, MessageType::GAME_END, serialized_end);

    // Sử dụng try-catch vì việc lấy match từ DB có thể fail
    try {
      // Lấy toàn bộ thông tin match từ database
      MatchModel match = data_storage_->getMatch(game_id);

      // Chuẩn bị GameLog message
      GameLogMessage game_log_msg;
      game_log_msg.game_id = game_id;

      // Chuyển timestamp C++ sang số (nanoseconds since epoch)
      game_log_msg.start_time = match.start_time.time_since_epoch().count();
      game_log_msg.end_time = match.end_time.time_since_epoch().count();

      // Thông tin IP và kết quả
      game_log_msg.white_ip = match.white_ip;
      game_log_msg.black_ip = match.black_ip;
      game_log_msg.winner = winner;
      game_log_msg.reason = reason;

      // Thu thập tất cả các nước đi từ match
      // For-each loop: duyệt qua vector match.moves
      for (const auto &move : match.moves) {
        game_log_msg.moves.push_back(move.uci_move); // Thêm vào vector
      }

      // Serialize và gửi log cho cả hai người chơi
      std::vector<uint8_t> serialized_log = game_log_msg.serialize();
      network_server_->sendPacketToUsername(
          player_white_name, MessageType::GAME_LOG, serialized_log);
      network_server_->sendPacketToUsername(
          player_black_name, MessageType::GAME_LOG, serialized_log);

      // Log thành công
      std::cout << "[GAME_LOG] Sent game log for " << game_id
                << " to both players." << std::endl;

    } catch (const std::exception &e) {
      std::cerr << "[GAME_LOG] Failed to send game log: " << e.what()
                << std::endl;
    }

    // Xóa game khỏi map `games` (giải phóng bộ nhớ)
    // Lưu ý: CHỈ xóa khỏi RAM, KHÔNG xóa khỏi database
    removeGame(game_id);
  }

  // Xử lý khi một client ngắt kết nối.
  void clientDisconnected(int client_fd) {
    std::string username = network_server_->getUsername(client_fd);
    std::shared_ptr<GameStatus> game = getGameByClientFd(client_fd);

    if (game != nullptr) {
      std::string game_id = game->game_id;
      std::string opponent_name;

      if (game->player_white_name == username) {
        opponent_name = game->player_black_name;
      } else if (game->player_black_name == username) {
        opponent_name = game->player_white_name;
      }

      // Send GameResultMessage to the opponent
      GameEndMessage game_end_msg;
      game_end_msg.game_id = game_id;
      game_end_msg.winner_username = opponent_name;
      game_end_msg.reason = "Opponent disconnected";
      game_end_msg.half_moves_count = game->getHalfMovesCount();
      network_server_->sendPacketToUsername(
          opponent_name, MessageType::GAME_END, game_end_msg.serialize());

      // Update points (disconnect = lose: -3, opponent wins: +3)
      int current_elo = data_storage_->getUserELO(username);
      int current_opponent_elo = data_storage_->getUserELO(opponent_name);
      int new_elo = (current_elo >= 3) ? current_elo - 3 : 0; // Lose -3
      int new_opponent_elo = current_opponent_elo + 3;        // Win +3

      data_storage_->updateUserELO(username, new_elo);
      data_storage_->updateUserELO(opponent_name, new_opponent_elo);

      // Remove the game from the system
      removeGame(game_id);
    }

    // Remove the client from the matchmaking queue
    removePlayerFromQueue(client_fd);
  }

  bool isGameOver(const std::string &game_id) {
    auto game = getGame(game_id);
    if (game)
      return game->isGameOver();
    return false;
  }

  std::string getGameFen(const std::string &game_id) {
    auto game = getGame(game_id);
    if (game)
      return game->getFen();
    return "";
  }

  std::string getGameCurrentTurn(const std::string &game_id) {
    auto game = getGame(game_id);
    if (game)
      return game->current_turn;
    return "";
  }

  std::string getGameWinner(const std::string &game_id) {
    auto game = getGame(game_id);
    if (game)
      return game->winner;
    return "";
  }

  std::string getGameResultReason(const std::string &game_id) {
    auto game = getGame(game_id);
    if (game)
      return game->getResultReason();
    return "";
  }

  uint16_t getGameHalfMovesCount(const std::string &game_id) {
    auto game = getGame(game_id);
    if (game)
      return game->getHalfMovesCount();
    return 0;
  }

  void addPlayerToQueue(int client_fd) {
    {
      std::lock_guard<std::mutex> lock(matchmaking_mutex);
      matchmaking_queue.push(client_fd);
    }
    cv.notify_one();
  }

  void removePlayerFromQueue(int client_fd) {
    std::lock_guard<std::mutex> lock(matchmaking_mutex);
    std::queue<int> new_queue;
    while (!matchmaking_queue.empty()) {
      int front = matchmaking_queue.front();
      matchmaking_queue.pop();
      if (front != client_fd) {
        new_queue.push(front);
      }
    }
    matchmaking_queue = new_queue;
  }

  void handleAutoMatchAccepted(int client_fd, const std::string &game_id) {
    std::lock_guard<std::mutex> lock(games_mutex);
    auto it = pending_games.find(game_id);
    if (it != pending_games.end()) {
      PendingGame &pending = it->second;
      if (client_fd == pending.player1_fd)
        pending.player1_accepted = true;
      else if (client_fd == pending.player2_fd)
        pending.player2_accepted = true;

      if (pending.player1_accepted && pending.player2_accepted) {
        // Both players accepted, game starts

        NetworkServer &network_server = NetworkServer::getInstance();

        // Notify both players about the game start
        GameStartMessage game_start_msg;
        game_start_msg.game_id = game_id;
        game_start_msg.player1_username =
            network_server.getUsername(pending.player1_fd);
        game_start_msg.player2_username =
            network_server.getUsername(pending.player2_fd);
        game_start_msg.starting_player_username =
            game_start_msg.player1_username; // Player 1 starts
        game_start_msg.fen = chess::constants::STARTPOS;

        std::vector<uint8_t> serialized = game_start_msg.serialize();

        network_server.sendPacket(pending.player1_fd, MessageType::GAME_START,
                                  serialized);
        network_server.sendPacket(pending.player2_fd, MessageType::GAME_START,
                                  serialized);

        // Remove from pending_games
        pending_games.erase(it);
      }
    }
  }

  void handleAutoMatchDeclined(int client_fd, const std::string &game_id) {
    std::lock_guard<std::mutex> lock(games_mutex);
    auto it = pending_games.find(game_id);
    if (it != pending_games.end()) {
      PendingGame pending = it->second;
      pending_games.erase(it);

      NetworkServer &network_server = NetworkServer::getInstance();

      // Notify the other player about the declination
      int other_fd = (client_fd == pending.player1_fd) ? pending.player2_fd
                                                       : pending.player1_fd;
      MatchDeclinedNotificationMessage decline_msg;
      decline_msg.game_id = game_id;
      std::vector<uint8_t> serialized = decline_msg.serialize();
      network_server.sendPacket(other_fd, decline_msg.getType(), serialized);

      // Requeue the other player
      matchmaking_queue.push(other_fd);
      cv.notify_one();
    }
  }

  bool isUserInGame(const std::string &username) {
    std::lock_guard<std::mutex> lock(games_mutex);
    for (const auto &game_pair : games) {
      std::shared_ptr<GameStatus> game = game_pair.second;
      if (game->player_white_name == username ||
          game->player_black_name == username) {
        return true;
      }
    }
    return false;
  }

  std::string getUserGameId(const std::string &username) {
    if (!isUserInGame(username)) {
      return "";
    }

    std::lock_guard<std::mutex> lock(games_mutex);
    for (const auto &game_pair : games) {
      std::shared_ptr<GameStatus> game = game_pair.second;
      if (game->player_white_name == username ||
          game->player_black_name == username) {
        return game_pair.first;
      }
    }
    return "";
  }

  std::string getOpponent(const std::string &game_id,
                          const std::string &player) {
    auto game = games.find(game_id);
    if (game == games.end())
      return "";

    if (game->second->player_white_name == player)
      return game->second->player_black_name;
    else if (game->second->player_black_name == player)
      return game->second->player_white_name;

    return "";
  }

  void endGameForSurrender(const std::string &game_id,
                           const std::string &surrendering_player) {
    DataStorage &datastorage = DataStorage::getInstance();
    GameStatus *game = getGame(game_id).get();

    std::string player_white_name = game->player_white_name;
    std::string player_black_name = game->player_black_name;

    std::string winner = (surrendering_player == player_white_name)
                             ? player_black_name
                             : player_white_name;
    std::string reason = "Player surrendered";

    // Gọi hàm updateMatchResult để cập nhật kết quả trận đấu
    datastorage.updateMatchResult(game_id, winner, reason);

    // Update ELO: surrendering player loses, opponent wins
    uint16_t white_elo = datastorage.getUserELO(player_white_name);
    uint16_t black_elo = datastorage.getUserELO(player_black_name);

    if (surrendering_player == player_white_name) {
      datastorage.updateUserELO(player_white_name,
                                (white_elo >= 3) ? white_elo - 3 : 0);
      datastorage.updateUserELO(player_black_name, black_elo + 3);
    } else {
      datastorage.updateUserELO(player_black_name,
                                (black_elo >= 3) ? black_elo - 3 : 0);
      datastorage.updateUserELO(player_white_name, white_elo + 3);
    }

    // Remove game
    removeGame(game_id);
  }
};

#endif // GAME_MANAGER_HPP
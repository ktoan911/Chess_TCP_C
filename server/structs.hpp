#ifndef STRUCTS_HPP
#define STRUCTS_HPP

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#include "../libraries/json.hpp"

using json = nlohmann::json;

// Thông tin người dùng
struct UserModel {
  std::string username; // Tên đăng nhập
  uint16_t elo;         // Điểm ELO

  // Chuyển sang JSON
  json serialize() const { return {{"elo", elo}}; }

  // Khôi phục từ JSON
  static UserModel deserialize(const std::string &username, const json &j) {
    return UserModel{username, j.at("elo").get<uint16_t>()};
  }
};

// Thông tin trận đấu
struct MatchModel {
  std::string game_id;        // ID trận đấu
  std::string white_username; // Người chơi trắng
  std::string black_username; // Người chơi đen
  std::string white_ip;       // IP người chơi trắng
  std::string black_ip;       // IP người chơi đen
  std::string start_fen;      // FEN bắt đầu

  // Thời gian bắt đầu/kết thúc
  std::chrono::time_point<std::chrono::system_clock> start_time;
  std::chrono::time_point<std::chrono::system_clock> end_time;

  // Thông tin nước đi
  struct Move {
    std::string uci_move; // Nước đi UCI
    std::string fen;      // FEN sau nước đi
    std::chrono::time_point<std::chrono::system_clock> move_time;
  };

  std::vector<Move> moves; // Danh sách nước đi
  std::string result;      // Kết quả (1-0, 0-1, 1/2-1/2)
  std::string reason;      // Lý do (checkmate, timeout, resign)

  // Chuyển sang JSON
  json serialize() const {
    json j;
    j["white_username"] = white_username;
    j["black_username"] = black_username;
    j["white_ip"] = white_ip;
    j["black_ip"] = black_ip;
    j["start_fen"] = start_fen;
    j["start_time"] = start_time.time_since_epoch().count();
    j["end_time"] = end_time.time_since_epoch().count();

    json moves_json;
    for (const auto &move : moves) {
      json move_json;
      move_json["uci_move"] = move.uci_move;
      move_json["fen"] = move.fen;
      move_json["move_time"] = move.move_time.time_since_epoch().count();
      moves_json.push_back(move_json);
    }
    j["moves"] = moves_json;

    j["result"] = result;
    j["reason"] = reason;

    return j;
  }

  // Khôi phục từ JSON
  static MatchModel deserialize(const std::string &game_id, const json &j) {
    MatchModel game;
    game.game_id = game_id;
    game.white_username = j.at("white_username").get<std::string>();
    game.black_username = j.at("black_username").get<std::string>();
    game.white_ip = j.value("white_ip", "");
    game.black_ip = j.value("black_ip", "");
    game.start_fen = j.at("start_fen").get<std::string>();

    game.start_time = std::chrono::time_point<std::chrono::system_clock>(
        std::chrono::nanoseconds(j.at("start_time").get<int64_t>()));
    game.end_time = std::chrono::time_point<std::chrono::system_clock>(
        std::chrono::nanoseconds(j.value("end_time", (int64_t)0)));

    for (const auto &move_json : j.at("moves")) {
      MatchModel::Move move;
      move.uci_move = move_json.at("uci_move").get<std::string>();
      move.fen = move_json.at("fen").get<std::string>();
      move.move_time = std::chrono::time_point<std::chrono::system_clock>(
          std::chrono::nanoseconds(move_json.at("move_time").get<int64_t>()));
      game.moves.push_back(move);
    }

    game.result = j.at("result").get<std::string>();
    game.reason = j.at("reason").get<std::string>();

    return game;
  }
};

// Trận đấu chờ chấp nhận
struct PendingGame {
  std::string game_id;
  int player1_fd;
  int player2_fd;
  bool player1_accepted;
  bool player2_accepted;

  PendingGame()
      : game_id(""), player1_fd(-1), player2_fd(-1), player1_accepted(false),
        player2_accepted(false) {}

  PendingGame(const std::string &id, int fd1, int fd2)
      : game_id(id), player1_fd(fd1), player2_fd(fd2), player1_accepted(false),
        player2_accepted(false) {}
};

// Thông tin client kết nối
struct ClientInfo {
  std::vector<uint8_t> buffer; // Bộ đệm nhận dữ liệu
  std::mutex mutex;            // Khóa bảo vệ buffer
  std::string username = "";   // Tên đăng nhập
};

#endif // STRUCTS_HPP

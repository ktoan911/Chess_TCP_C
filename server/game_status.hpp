#ifndef GAME_HPP
#define GAME_HPP

#include <algorithm>
#include <string>

#include "../chess_engine/chess.hpp"

/**
 * @class Game
 * @brief Quản lý trạng thái và logic của một ván cờ.
 *
 * Lớp này bao gồm các thông tin về người chơi, trạng thái bàn cờ,
 * lượt chơi hiện tại, và các phương thức để thực hiện nước đi,
 * kiểm tra trạng thái kết thúc của trò chơi, và các thông tin liên quan khác.
 */
class GameStatus {
public:
  std::string game_id;
  std::string player_white_name;
  std::string player_black_name;
  std::string current_turn;

  std::string winner;

  GameStatus(const std::string &id, const std::string &p1,
             const std::string &p2, const std::string &fen)
      : game_id(id), player_white_name(p1), player_black_name(p2), board(fen),
        is_over(false), winner("") {
    chess::Color current_turn_color = board.sideToMove();
    bool isWhiteTurn = current_turn_color == chess::Color::WHITE;
    current_turn = isWhiteTurn ? player_white_name : player_black_name;
  }

  bool makeMove(const std::string &uci_move) {
    chess::Move move = chess::uci::uciToMove(board, uci_move);

    if (!isValidMove(board, move))
      return false;

    board.makeMove(move);
    half_moves_count++;

    // Kiểm tra kết quả trò chơi
    std::tie(reason, result) = board.isGameOver();

    if (result == chess::GameResult::NONE) {
      // Chuyển lượt chơi
      toggleTurn();
    } else {
      is_over = true;
      if (result == chess::GameResult::DRAW)
        winner = "<0>";
      else if (result == chess::GameResult::LOSE)
        winner = current_turn;
    }

    return true;
  }

  bool isInCheck() {
    // Get the king's square for the current turn
    chess::Color current_turn_color = (current_turn == player_white_name)
                                          ? chess::Color::WHITE
                                          : chess::Color::BLACK;
    chess::Square king = board.kingSq(current_turn_color);

    // Determine the opponent's color
    chess::Color opponent = (current_turn_color == chess::Color::WHITE)
                                ? chess::Color::BLACK
                                : chess::Color::WHITE;

    // Check if the king's square is attacked by the opponent
    return board.isAttacked(king, opponent);
  }

  bool isGameOver() { return is_over; }

  std::string getFen() { return board.getFen(); }

  std::string getResult() {
    switch (result) {
    case chess::GameResult::LOSE:
      return winner + " wins";
    case chess::GameResult::DRAW:
      return "draw";
    default:
      return "";
    }
  }

  std::string getResultReason() {
    switch (reason) {
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

  int getHalfMovesCount() const { return half_moves_count; }

private:
  bool is_over;

  chess::Board board;
  chess::GameResult result = chess::GameResult::NONE;
  chess::GameResultReason reason = chess::GameResultReason::NONE;
  int half_moves_count = 0;

  bool isValidMove(const chess::Board &board, const chess::Move &move) {
    if (move == chess::Move::NO_MOVE) {
      return false;
    }

    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, board);

    if (std::find(legal_moves.begin(), legal_moves.end(), move) ==
        legal_moves.end()) {
      return false;
    }

    return true;
  }

  void toggleTurn() {
    current_turn = (current_turn == player_white_name) ? player_black_name
                                                       : player_white_name;
  }
};

#endif // GAME_HPP

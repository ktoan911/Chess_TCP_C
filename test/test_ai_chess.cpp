#include <iostream>
#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <random>
#include <cctype>

#include "../chess_engine/chess.hpp"
#include "../chess_engine/chess_bot.hpp"
#include "../client/board_display.hpp"

// Constants
const int MAX_DEPTH = 4;

std::string getResultReason(chess::GameResultReason reason)
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

int main()
{
    std::srand(static_cast<unsigned>(std::time(0)));

    const char *initFen = chess::constants::STARTPOS;
    chess::Board board(initFen);
    chess::Color current_turn = board.sideToMove();
    bool isWhiteTurn = (current_turn == chess::Color::WHITE);

    // AI màu đen
    chess::Color aiColor = chess::Color::BLACK;

    while (true)
    {
        std::cout << std::endl;
        board_display::printBoard(board.getFen(), isWhiteTurn);

        // Check if the game is over
        auto gameStatus = board.isGameOver();
        if (gameStatus.second != chess::GameResult::NONE)
        {
            std::cout << "Game Over: ";
            switch (gameStatus.second)
            {
            case chess::GameResult::LOSE:
                std::cout << (isWhiteTurn ? "Black" : "White") << " wins! Due to " << getResultReason(gameStatus.first) << std::endl;
                break;
            case chess::GameResult::DRAW:
                std::cout << "Draw! Due to " << getResultReason(gameStatus.first) << std::endl;
                break;
            default:
                std::cout << "None" << std::endl;
                break;
            }
            std::cout << "Final position: " << board.getFen() << std::endl;
            std::cout << std::endl;
            break;
        }

        chess::Move move;
        if (isWhiteTurn)
        {
            chess::Movelist moves;
            chess::movegen::legalmoves(moves, board);
            if (moves.empty())
                break;
            move = moves[std::rand() % moves.size()];
            std::cout << "White plays: " << chess::uci::moveToUci(move) << std::endl;
            std::cout << "Press Enter to continue...";
            std::cin.ignore();
        }
        else
        {
            move = ChessBot::getInstance().findBestMove(board.getFen(), aiColor, 4);
            if (move == chess::Move::NO_MOVE)
            {
                std::cout << "Black (AI) has no legal moves." << std::endl;
                break;
            }
            std::cout << "Black (AI) plays: " << chess::uci::moveToUci(move) << std::endl;
        }

        board.makeMove(move);
        isWhiteTurn = !isWhiteTurn;
    }

    return 0;
}

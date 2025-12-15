#ifndef CHESS_BOT_HPP
#define CHESS_BOT_HPP

#include "chess.hpp"

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

#pragma region Opening Book

struct BookMove
{
    std::string moveString; // Move in UCI format, e.g., "e2e4"
    int numTimesPlayed;     // Number of times this move was played
};

// Define a hash function for strings to use with unordered_map
struct StringHash
{
    std::size_t operator()(const std::string &key) const
    {
        return std::hash<std::string>()(key);
    }
};

class OpeningBookManager
{
public:
    OpeningBookManager(const std::string &filePath)
    {
        loadBook(filePath);
    }

    // Attempts to get a book move for the current board.
    // Returns true and sets 'move' if a book move is found.
    // Returns false otherwise.
    bool tryGetBookMove(const chess::Board &board, chess::Move &move) const
    {
        std::string fen = board.getFen(false); // Exclude move counters
        // Remove the last two fields (halfmove clock and fullmove number)
        fen = removeMoveCountersFromFen(fen);

        auto it = bookMoves.find(fen);
        if (it == bookMoves.end())
        {
            return false; // No book moves for this position
        }

        const std::vector<BookMove> &moves = it->second;
        if (moves.empty())
        {
            return false; // No moves listed for this position
        }

        // Calculate the total weight
        int totalWeight = 0;
        for (const auto &bm : moves)
        {
            totalWeight += bm.numTimesPlayed;
        }

        if (totalWeight == 0)
        {
            return false; // Avoid division by zero
        }

        // Generate a random number in [1, totalWeight]
        std::uniform_int_distribution<int> dist(1, totalWeight);
        int randomValue = dist(rng);

        // Select the move based on the random value
        int cumulative = 0;
        for (const auto &bm : moves)
        {
            cumulative += bm.numTimesPlayed;
            if (randomValue <= cumulative)
            {
                // Convert the move string to a Move object
                chess::Move candidateMove = chess::uci::uciToMove(board, bm.moveString);
                if (candidateMove != chess::Move::NO_MOVE)
                {
                    move = candidateMove;
                    return true;
                }
                break; // Invalid move string; proceed to next
            }
        }

        return false; // Fallback
    }

private:
    // Maps FEN strings to their corresponding book moves
    std::unordered_map<std::string, std::vector<BookMove>, StringHash> bookMoves;

    // Random number generator
    mutable std::mt19937 rng{std::random_device{}()};

    // Loads the opening book from the specified file
    void loadBook(const std::string &filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            std::cerr << "Error: Unable to open opening book file: " << filePath << std::endl;
            return;
        }

        std::string line;
        std::string currentFEN;

        while (std::getline(file, line))
        {
            // Trim leading and trailing whitespace
            trim(line);
            if (line.empty())
                continue;

            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == "pos")
            {
                // The rest of the line is the FEN string
                std::string fenPart;
                std::getline(iss, fenPart);
                trim(fenPart);
                currentFEN = removeMoveCountersFromFen(fenPart);
                // Initialize the vector for this FEN if not already
                if (bookMoves.find(currentFEN) == bookMoves.end())
                {
                    bookMoves[currentFEN] = std::vector<BookMove>();
                }
            }
            else
            {
                // It's a move line: <move> <count>
                if (currentFEN.empty())
                {
                    std::cerr << "Warning: Move found before any position definition." << std::endl;
                    continue;
                }

                std::string moveStr;
                int count;
                std::istringstream moveStream(line);
                moveStream >> moveStr >> count;
                if (moveStr.empty() || moveStream.fail())
                {
                    std::cerr << "Warning: Invalid move line: " << line << std::endl;
                    continue;
                }

                bookMoves[currentFEN].emplace_back(BookMove{moveStr, count});
            }
        }

        file.close();
    }

    // Removes the last two fields from the FEN string
    std::string removeMoveCountersFromFen(const std::string &fen) const
    {
        // FEN fields: piece placement, active color, castling availability, en passant target square, halfmove clock, fullmove number
        // We want to exclude the last two fields
        size_t pos = fen.find_last_of(' ');
        if (pos == std::string::npos)
            return fen; // Invalid FEN, return as is

        pos = fen.find_last_of(' ', pos - 1);
        if (pos == std::string::npos)
            return fen; // Invalid FEN, return as is

        return fen.substr(0, pos);
    }

    // Trims leading and trailing whitespace from a string
    void trim(std::string &s) const
    {
        // Trim leading spaces
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                        [](unsigned char ch)
                                        { return !std::isspace(ch); }));
        // Trim trailing spaces
        s.erase(std::find_if(s.rbegin(), s.rend(),
                             [](unsigned char ch)
                             { return !std::isspace(ch); })
                    .base(),
                s.end());
    }
};

#pragma endregion Opening Book

#pragma region Chess Bot
class ChessBot
{
public:
    static ChessBot &getInstance()
    {
        static ChessBot instance;
        return instance;
    }

    chess::Move findBestMove(const std::string &fen, chess::Color aiColor, int level = 2)
    {
        return findBestMoveInternal(fen, level, aiColor);
    }

private:
    ChessBot() {}
    ChessBot(const ChessBot &) = delete;
    ChessBot &operator=(const ChessBot &) = delete;

    // Piece values
    const int PAWN_VALUE = 100;
    const int KNIGHT_VALUE = 320;
    const int BISHOP_VALUE = 330;
    const int ROOK_VALUE = 500;
    const int QUEEN_VALUE = 900;
    const int KING_VALUE = 0;
    const int CHECKMATE_VALUE = 100000;

    // Piece-Square Tables
    const int pawnTable[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        5, 10, 10, -20, -20, 10, 10, 5,
        5, -5, -10, 0, 0, -10, -5, 5,
        0, 0, 0, 20, 20, 0, 0, 0,
        5, 5, 10, 25, 25, 10, 5, 5,
        10, 10, 20, 30, 30, 20, 10, 10,
        50, 50, 50, 50, 50, 50, 50, 50,
        0, 0, 0, 0, 0, 0, 0, 0};

    const int knightTable[64] = {
        -50, -40, -30, -30, -30, -30, -40, -50,
        -40, -20, 0, 5, 5, 0, -20, -40,
        -30, 5, 10, 15, 15, 10, 5, -30,
        -30, 0, 15, 20, 20, 15, 0, -30,
        -30, 5, 15, 20, 20, 15, 5, -30,
        -30, 0, 10, 15, 15, 10, 0, -30,
        -40, -20, 0, 0, 0, 0, -20, -40,
        -50, -40, -30, -30, -30, -30, -40, -50};

    const int bishopTable[64] = {
        -20, -10, -10, -10, -10, -10, -10, -20,
        -10, 5, 0, 0, 0, 0, 5, -10,
        -10, 10, 10, 10, 10, 10, 10, -10,
        -10, 0, 10, 10, 10, 10, 0, -10,
        -10, 5, 5, 10, 10, 5, 5, -10,
        -10, 0, 5, 10, 10, 5, 0, -10,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -20, -10, -10, -10, -10, -10, -10, -20};

    const int rookTable[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        5, 10, 10, 10, 10, 10, 10, 5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        0, 0, 0, 5, 5, 0, 0, 0};

    const int queenTable[64] = {
        -20, -10, -10, -5, -5, -10, -10, -20,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -10, 5, 5, 5, 5, 5, 0, -10,
        0, 0, 5, 5, 5, 5, 0, -5,
        -5, 0, 5, 5, 5, 5, 0, -5,
        -10, 0, 5, 5, 5, 5, 0, -10,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -20, -10, -10, -5, -5, -10, -10, -20};

    const int kingTable[64] = {
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -20, -30, -30, -40, -40, -30, -30, -20,
        -10, -20, -20, -20, -20, -20, -20, -10,
        20, 20, 0, 0, 0, 0, 20, 20,
        20, 30, 10, 0, 0, 10, 30, 20};

    // Evaluation Function
    int evaluate(const chess::Board &board)
    {
        int score = 0;
        for (int sq = 0; sq < 64; ++sq)
        {
            chess::Square square = chess::Square(sq);
            chess::Piece piece = board.at<chess::Piece>(square);

            if (piece == chess::Piece::NONE)
                continue;

            int pieceValue = 0;
            int pst = 0;
            bool isWhite = (piece.color() == chess::Color::WHITE);
            // Extract the underlying enum value for switching
            chess::PieceType::underlying type = piece.type().internal();

            switch (type)
            {
            case chess::PieceType::PAWN:
                pieceValue = PAWN_VALUE;
                pst = pawnTable[isWhite ? sq : 63 - sq];
                break;
            case chess::PieceType::KNIGHT:
                pieceValue = KNIGHT_VALUE;
                pst = knightTable[isWhite ? sq : 63 - sq];
                break;
            case chess::PieceType::BISHOP:
                pieceValue = BISHOP_VALUE;
                pst = bishopTable[isWhite ? sq : 63 - sq];
                break;
            case chess::PieceType::ROOK:
                pieceValue = ROOK_VALUE;
                pst = rookTable[isWhite ? sq : 63 - sq];
                break;
            case chess::PieceType::QUEEN:
                pieceValue = QUEEN_VALUE;
                pst = queenTable[isWhite ? sq : 63 - sq];
                break;
            case chess::PieceType::KING:
                pieceValue = KING_VALUE;
                pst = kingTable[isWhite ? sq : 63 - sq];
                break;
            default:
                break;
            }
            if (isWhite)
                score += pieceValue + pst;
            else
                score -= (pieceValue + pst);
        }
        return score;
    }

    // Helper function to get piece value
    int getPieceValue(chess::PieceType type)
    {
        switch (type.internal())
        {
        case chess::PieceType::PAWN:
            return PAWN_VALUE;
        case chess::PieceType::KNIGHT:
            return KNIGHT_VALUE;
        case chess::PieceType::BISHOP:
            return BISHOP_VALUE;
        case chess::PieceType::ROOK:
            return ROOK_VALUE;
        case chess::PieceType::QUEEN:
            return QUEEN_VALUE;
        case chess::PieceType::KING:
            return KING_VALUE;
        default:
            return 0;
        }
    }

    // Function to compare moves using MVV-LVA ordering
    bool mvvLvaComparator(const chess::Move &a, const chess::Move &b, const chess::Board &board) {
        chess::Piece capturedA = board.at<chess::Piece>(a.to());
        chess::Piece capturedB = board.at<chess::Piece>(b.to());
        
        // Check if both moves are captures
        bool isCaptureA = capturedA != chess::Piece::NONE;
        bool isCaptureB = capturedB != chess::Piece::NONE;
        
        if (isCaptureA && isCaptureB) {
            int valueA = getPieceValue(capturedA.type());
            int valueB = getPieceValue(capturedB.type());
            // Sort by higher victim value first
            if (valueA != valueB)
                return valueA > valueB;
            
            // If victim values are equal, sort by lower aggressor value first
            chess::Piece movingPieceA = board.at<chess::Piece>(a.from());
            chess::Piece movingPieceB = board.at<chess::Piece>(b.from());
            return getPieceValue(movingPieceA.type()) < getPieceValue(movingPieceB.type());
        }
        
        // Prioritize captures over non-captures
        if (isCaptureA != isCaptureB)
            return isCaptureA;
        
        return false; // If neither are captures, maintain current order
    }

    int count = 0;

    // Minimax with Alpha-Beta Pruning
    int minimax(chess::Board &board, int depth, int alpha, int beta, bool maximizingPlayer, chess::Color aiColor)
    {
        count++;

        if (depth == 0)
        {
            return evaluate(board);
        }

        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);

        if (moves.empty())
        {
            auto [reason, result] = board.isGameOver();
            if (result == chess::GameResult::LOSE)
                return maximizingPlayer ? -CHECKMATE_VALUE : CHECKMATE_VALUE;
            else
                return 0;
        }

        // Apply MVV-LVA Comparator
        std::sort(moves.begin(), moves.end(), [&](const chess::Move &a, const chess::Move &b) -> bool {
            return mvvLvaComparator(a, b, board);
        });

        if (maximizingPlayer)
        {
            int maxEval = std::numeric_limits<int>::min();
            for (auto &move : moves)
            {
                board.makeMove(move);
                int eval = minimax(board, depth - 1, alpha, beta, false, aiColor);
                board.unmakeMove(move);
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                if (beta <= alpha)
                    break;
            }
            return maxEval;
        }
        else
        {
            int minEval = std::numeric_limits<int>::max();
            for (auto &move : moves)
            {
                board.makeMove(move);
                int eval = minimax(board, depth - 1, alpha, beta, true, aiColor);
                board.unmakeMove(move);
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                if (beta <= alpha)
                    break;
            }
            return minEval;
        }
    }

    chess::Move findBestMoveInternal(const std::string &fen, int depth, chess::Color aiColor)
    {
        count = 0;
        chess::Board board(fen);

        // Initialize the OpeningBookManager with the path to Book.txt
        OpeningBookManager openingBook("../chess_engine/Book.txt");

        // Attempt to get a book move
        chess::Move bestMove = chess::Move::NO_MOVE;
        bool hasBookMove = openingBook.tryGetBookMove(board, bestMove);

        if (hasBookMove && bestMove != chess::Move::NO_MOVE)
        {
            std::cout << "Book Move: " << chess::uci::moveToUci(bestMove) << std::endl;
            return bestMove;
        }

        chess::Movelist moves;
        chess::movegen::legalmoves(moves, board);
        // Apply MVV-LVA Comparator
        std::sort(moves.begin(), moves.end(), [&](const chess::Move &a, const chess::Move &b) -> bool {
            return mvvLvaComparator(a, b, board);
        });
        int bestValue;

        if (aiColor == chess::Color::WHITE)
        {
            bestValue = std::numeric_limits<int>::min();
            for (auto &move : moves)
            {
                board.makeMove(move);
                int boardValue = minimax(board, depth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), false, aiColor);
                board.unmakeMove(move);
                if (boardValue > bestValue)
                {
                    bestValue = boardValue;
                    bestMove = move;
                }
            }
        }
        else
        {
            bestValue = std::numeric_limits<int>::max();
            for (auto &move : moves)
            {
                board.makeMove(move);
                int boardValue = minimax(board, depth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), true, aiColor);
                board.unmakeMove(move);
                if (boardValue < bestValue)
                {
                    bestValue = boardValue;
                    bestMove = move;
                }
            }
        }

        std::cout << "Nodes evaluated: " << count << std::endl;
        std::cout << "Best move value: " << std::abs(bestValue) << std::endl;
        return bestMove;
    }
};
#pragma endregion Chess Bot

#endif // CHESS_BOT_HPP
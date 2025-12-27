// board_display.hpp
#ifndef BOARD_DISPLAY_HPP
#define BOARD_DISPLAY_HPP

#include <iostream>
#include <string>
#include "../chess_engine/chess.hpp"

// ANSI color codes
const std::string RESET = "\033[0m";
const std::string WHITE_COLOR = "\033[96m"; // Cyan
const std::string BLACK_COLOR = "\033[31m"; // Red
const std::string LABEL_COLOR = "\033[34m"; // Orange

#define WHITE_SQUARE "\033[37m█\033[0m"
#define BLACK_SQUARE "\033[90m█\033[0m"
#define SPACE " "

/**
 * @brief namespace chứa các hàm hỗ trợ hiển thị bàn cờ trong ứng dụng TCP_Chess.
 */
namespace board_display
{

    bool isWhitePiece(const chess::Piece &piece)
    {
        return piece.color() == chess::Color::WHITE;
    }

    bool isBlackPiece(const chess::Piece &piece)
    {
        return piece.color() == chess::Color::BLACK;
    }

    /**
     * @brief In một dòng của bàn cờ với các thành phần và màu sắc được định dạng.
     * 
     * Giải thích cấu trúc:
     * 
     * - Một ô (cell): Mỗi ô trên bàn cờ được chia thành các phần nhỏ để hiển thị ký hiệu quân cờ và màu sắc. Mỗi ô có độ cao nhất định được chia thành các subLine để tạo độ sâu và định dạng cho việc hiển thị.
     * 
     * - Một dòng (line): Một dòng trên bàn cờ bao gồm nhiều ô, mỗi ô đại diện cho một vị trí cụ thể trên bàn cờ. Hàm `printLine` xử lý việc in từng dòng, bao gồm việc xác định vị trí quân cờ, áp dụng màu sắc phù hợp, và sắp xếp các ô theo thứ tự cần thiết (có thể lật bàn cờ nếu cần).
     * 
     * @param iLine Số thứ tự của dòng cần in.
     * @param pchColor1 Màu sắc được áp dụng cho ô thứ nhất trong cặp ô.
     * @param pchColor2 Màu sắc được áp dụng cho ô thứ hai trong cặp ô.
     * @param board Bản đồ hiện tại của bàn cờ, chứa trạng thái các quân cờ.
     * @param flip Cờ đánh dấu xem bàn cờ có được lật ngược không.
     */
    void printLine(int iLine, const char *pchColor1, const char *pchColor2, const chess::Board &board, bool flip)
    {
        const int CELL = 6; // Number of characters per cell vertically

        for (int subLine = 0; subLine < CELL / 2; subLine++)
        {
            if (subLine == 1)
            {
                std::cout << LABEL_COLOR << (flip ? 8 - iLine : iLine + 1) << RESET << "   ";
            }
            else
            {
                std::cout << "    ";
            }

            for (int iPair = 0; iPair < 4; iPair++)
            {
                bool hasPiece1 = false; // Flag for the first square
                for (int subColumn = 0; subColumn < CELL; subColumn++)
                {
                    if (subLine == 1 && subColumn == 2)
                    {
                        chess::Square sq1 = flip ? chess::Square((7 - iLine) * 8 + (7 - iPair * 2)) : chess::Square(iLine * 8 + iPair * 2);
                        chess::Piece piece1 = board.at(sq1);

                        if (piece1 != chess::Piece::NONE)
                        {
                            if (isWhitePiece(piece1))
                                std::cout << WHITE_COLOR << piece1.getSymbol() << RESET;
                            else if (isBlackPiece(piece1))
                                std::cout << BLACK_COLOR << piece1.getSymbol() << RESET;
                            else
                                std::cout << pchColor1;
                            hasPiece1 = true;
                        }
                        else
                        {
                            std::cout << pchColor1;
                        }
                    }
                    else if (subLine == 1 && subColumn == 3)
                    {
                        if (hasPiece1)
                            std::cout << SPACE;
                        else
                            std::cout << pchColor1;
                    }
                    else
                    {
                        std::cout << pchColor1;
                    }
                }

                bool hasPiece2 = false; // Flag for the second square
                for (int subColumn = 0; subColumn < CELL; subColumn++)
                {
                    if (subLine == 1 && subColumn == 2)
                    {
                        chess::Square sq2 = flip ? chess::Square((7 - iLine) * 8 + (7 - iPair * 2 - 1)) : chess::Square(iLine * 8 + iPair * 2 + 1);
                        chess::Piece piece2 = board.at(sq2);

                        if (piece2 != chess::Piece::NONE)
                        {
                            if (isWhitePiece(piece2))
                                std::cout << WHITE_COLOR << piece2.getSymbol() << RESET;
                            else if (isBlackPiece(piece2))
                                std::cout << BLACK_COLOR << piece2.getSymbol() << RESET;
                            else
                                std::cout << pchColor2;
                            hasPiece2 = true;
                        }
                        else
                        {
                            std::cout << pchColor2;
                        }
                    }
                    else if (subLine == 1 && subColumn == 3)
                    {
                        if (hasPiece2)
                            std::cout << SPACE;
                        else
                            std::cout << pchColor2;
                    }
                    else
                    {
                        std::cout << pchColor2;
                    }
                }
            }

            if (subLine == 1)
            {
                std::cout << "   " << LABEL_COLOR << (flip ? 8 - iLine : iLine + 1) << RESET;
            }
            std::cout << "\n";
        }
    }

    void printBoard(const std::string &fen, bool flip)
    {
        chess::Board board(fen);

        std::cout << "\n========================================================\n";

        if (flip)
        {
            std::cout << "      "
                      << LABEL_COLOR << "H" << RESET << "     "
                      << LABEL_COLOR << "G" << RESET << "     "
                      << LABEL_COLOR << "F" << RESET << "     "
                      << LABEL_COLOR << "E" << RESET << "     "
                      << LABEL_COLOR << "D" << RESET << "     "
                      << LABEL_COLOR << "C" << RESET << "     "
                      << LABEL_COLOR << "B" << RESET << "     "
                      << LABEL_COLOR << "A" << RESET << "\n\n";
        }
        else
        {
            std::cout << "      "
                      << LABEL_COLOR << "A" << RESET << "     "
                      << LABEL_COLOR << "B" << RESET << "     "
                      << LABEL_COLOR << "C" << RESET << "     "
                      << LABEL_COLOR << "D" << RESET << "     "
                      << LABEL_COLOR << "E" << RESET << "     "
                      << LABEL_COLOR << "F" << RESET << "     "
                      << LABEL_COLOR << "G" << RESET << "     "
                      << LABEL_COLOR << "H" << RESET << "\n\n";
        }

        for (int iLine = 7; iLine >= 0; iLine--)
        {
            if (iLine % 2 == 0)
            {
                printLine(iLine, BLACK_SQUARE, WHITE_SQUARE, board, flip);
            }
            else
            {
                printLine(iLine, WHITE_SQUARE, BLACK_SQUARE, board, flip);
            }
        }

        if (flip)
        {
            std::cout << "\n      "
                      << LABEL_COLOR << "H" << RESET << "     "
                      << LABEL_COLOR << "G" << RESET << "     "
                      << LABEL_COLOR << "F" << RESET << "     "
                      << LABEL_COLOR << "E" << RESET << "     "
                      << LABEL_COLOR << "D" << RESET << "     "
                      << LABEL_COLOR << "C" << RESET << "     "
                      << LABEL_COLOR << "B" << RESET << "     "
                      << LABEL_COLOR << "A" << RESET << "\n";
        }
        else
        {
            std::cout << "\n      "
                      << LABEL_COLOR << "A" << RESET << "     "
                      << LABEL_COLOR << "B" << RESET << "     "
                      << LABEL_COLOR << "C" << RESET << "     "
                      << LABEL_COLOR << "D" << RESET << "     "
                      << LABEL_COLOR << "E" << RESET << "     "
                      << LABEL_COLOR << "F" << RESET << "     "
                      << LABEL_COLOR << "G" << RESET << "     "
                      << LABEL_COLOR << "H" << RESET << "\n";
        }

        std::cout << "========================================================\n\n";
    }
} // namespace board_display

#endif // BOARD_DISPLAY_HPP
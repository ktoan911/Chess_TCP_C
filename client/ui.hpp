// ui.hpp
#ifndef UI_HPP
#define UI_HPP

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include "../libraries/tabulate.hpp"
#include "../common/message.hpp"
#include "board_display.hpp"

#define RESET "\033[0m"
#define CYAN "\033[96m"   // Cyan
#define RED "\033[31m"    // Red
#define GREEN "\033[32m"  // Green
#define YELLOW "\033[33m" // Yellow
#define BLUE "\033[34m"   // Blue

/**
 * @namespace UI
 * @brief Chứa các chức năng giao diện người dùng.
 * 
 * Phiên bản single-threaded: Các hàm chỉ hiển thị prompt,
 * không chờ input (input được xử lý bởi event loop).
 */
namespace UI
{
    // Clear the console
    void clearConsole()
    {
        std::cout << "\033[2J\033[H" << std::flush;
    }

    // Print the logo
    void printLogo(void)
    {
        std::cout << std::endl;
        std::cout << BLUE << "=============================================================\n"
                  << RESET;
        std::cout << "   _______   _____  _____     _____  _                       \n";
        std::cout << "  |__   __| / ____||  __ \\   / ____|| |                     \n";
        std::cout << "     | |   | |     | |__) | | |     | |__    ___  ___  ___   \n";
        std::cout << "     | |   | |     |  ___/  | |     | '_ \\  / _ \\/ __|/ __|\n";
        std::cout << "     | |   | |____ | |      | |____ | | | ||  __/\\__ \\\\__ \\\n";
        std::cout << "     |_|    \\_____||_|       \\_____||_| |_| \\___||___/|___/\n";
        std::cout << BLUE << "=============================================================\n"
                  << RESET;
    }

    // Print error message in red
    void printErrorMessage(const std::string &message)
    {
        std::cout << std::endl;
        std::cerr << RED << message << RESET << std::endl;
    }

    // Print success message in green
    void printSuccessMessage(const std::string &message)
    {
        std::cout << std::endl;
        std::cout << GREEN << message << RESET << std::endl;
    }

    // Print info message in cyan
    void printInfoMessage(const std::string &message)
    {
        std::cout << std::endl;
        std::cout << CYAN << message << RESET << std::endl;
    }

    // Display the main menu prompt (no waiting for input)
    void displayInitialMenuPrompt()
    {
        std::cout << "\n========= Main menu =========" << std::endl;
        std::cout << "Chọn hành động: " << std::endl;
        std::cout << "  1. Đăng ký" << std::endl;
        std::cout << "  2. Đăng nhập" << std::endl;
        std::cout << "  3. Thoát" << std::endl;
        std::cout << "> " << std::flush;
    }

    // Display register prompt
    void displayRegisterPrompt()
    {
        std::cout << "\n========= Register =========" << std::endl;
        std::cout << "Username: " << std::flush;
    }

    // Display login prompt
    void displayLoginPrompt()
    {
        std::cout << "\n========= Login =========" << std::endl;
        std::cout << "Username: " << std::flush;
    }

    // Display the game menu prompt
    void displayGameMenuPrompt()
    {
        std::cout << "\n========= Game menu =========" << std::endl;
        std::cout << "Chọn hành động: " << std::endl;
        std::cout << "  1. Ghép trận tự động" << std::endl;
        std::cout << "  2. Danh sách người chơi trực tuyến" << std::endl;
        std::cout << "  3. Trở về" << std::endl;
        std::cout << "> " << std::flush;
    }

    // Display auto match options prompt
    void displayAutoMatchOptionsPrompt(const std::string& opponent, uint16_t elo, const std::string& game_id)
    {
        std::cout << "\n========= Tìm thấy trận! =========" << std::endl;
        std::cout << "Đối thủ: " << opponent << std::endl;
        std::cout << "ELO: " << elo << std::endl;
        std::cout << "Game ID: " << game_id << std::endl;
        std::cout << "\nChọn hành động: " << std::endl;
        std::cout << "  1. Chấp nhận" << std::endl;
        std::cout << "  2. Từ chối" << std::endl;
        std::cout << "> " << std::flush;
    }

    // Display challenge decision prompt
    void displayChallengeDecisionPrompt(const std::string& challenger, uint16_t elo)
    {
        std::cout << "\n========= Thách đấu =========" << std::endl;
        std::cout << "Người chơi: " << challenger << " (ELO: " << elo << ") muốn thách đấu bạn!" << std::endl;
        std::cout << "\nChọn hành động: " << std::endl;
        std::cout << "  1. Chấp nhận" << std::endl;
        std::cout << "  2. Từ chối" << std::endl;
        std::cout << "> " << std::flush;
    }

    // Display player list
    void displayPlayerList(const std::vector<PlayerListMessage::Player>& players, const std::string& current_user)
    {
        std::cout << "\n========= Danh sách người chơi =========" << std::endl;
        
        if (players.empty())
        {
            std::cout << "(Không có người chơi nào online)" << std::endl;
        }
        else
        {
            for (const auto &player : players)
            {
                std::string status = player.in_game ? "[Đang chơi]" : "[Online]";
                std::string marker = (player.username == current_user) ? " (Bạn)" : "";
                std::cout << "  - " << player.username << " (ELO: " << player.elo << ") " 
                          << status << marker << std::endl;
            }
        }
        
        std::cout << "\n===== Lựa chọn =====" << std::endl;
        std::cout << "1. Thách đấu người chơi khác" << std::endl;
        std::cout << "2. Quay lại" << std::endl;
        std::cout << "> " << std::flush;
    }

    // Display challenge input prompt
    void displayChallengeInputPrompt()
    {
        std::cout << "Nhập tên người chơi muốn thách đấu: " << std::flush;
    }

    // Display move prompt
    void displayMovePrompt()
    {
        std::cout << "Nhập nước đi (VD: e2e4) hoặc 'surrender' để đầu hàng: " << std::flush;
    }

    // Display waiting message
    void displayWaitingOpponentMove()
    {
        printInfoMessage("Đang chờ đối thủ ra nước đi...");
    }

    // Display waiting for auto match
    void displayWaitingAutoMatch()
    {
        printInfoMessage("Đang tìm đối thủ...");
    }

    // Display waiting for challenge response
    void displayWaitingChallengeResponse()
    {
        printInfoMessage("Đã gửi yêu cầu thách đấu. Đang chờ phản hồi...");
    }

    // Display the chess board
    void showBoard(const std::string &fen, bool flip = false)
    {
        board_display::printBoard(fen, flip);
    }

    // Display game start info
    void displayGameStart(const std::string& game_id, const std::string& player1, 
                          const std::string& player2, const std::string& starting_player)
    {
        printInfoMessage("Trò chơi đã bắt đầu!");
        std::cout << "Game ID: " << game_id << std::endl;
        std::cout << "Player 1 (White): " << player1 << std::endl;
        std::cout << "Player 2 (Black): " << player2 << std::endl;
        std::cout << "Người đi trước: " << starting_player << std::endl;
    }

    // Display game end
    void displayGameEnd(const std::string& game_id, const std::string& winner, 
                        const std::string& reason, uint16_t half_moves)
    {
        printInfoMessage("Trò chơi đã kết thúc!");
        std::cout << "Game ID: " << game_id << std::endl;
        std::cout << "Người thắng: " << winner << std::endl;
        std::cout << "Lý do: " << reason << std::endl;
        std::cout << "Số nước đi: " << half_moves << std::endl;
    }

} // namespace UI

#endif // UI_HPP
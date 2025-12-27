// ui.hpp
#ifndef UI_HPP
#define UI_HPP

#include <iostream>
#include <string>
#include <cstring>

#include "../libraries/tabulate.hpp"

#include "message_handler.hpp"
#include "board_display.hpp"
#include "input_handler.hpp"

#define RESET "\033[0m"
#define CYAN "\033[96m"   // Cyan
#define RED "\033[31m"    // Red
#define GREEN "\033[32m"  // Green
#define YELLOW "\033[33m" // Yellow
#define BLUE "\033[34m"   // Blue

/**
 * @namespace UI
 * @brief Chứa các chức năng giao diện người dùng như hiển thị menu, thông báo và tương tác với người dùng.
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

    // Display the main menu
    std::string displayInitialMenu()
    {
        std::cout << "\n========= Main menu =========" << std::endl;
        std::cout << "Chọn hành động: " << std::endl;
        std::cout << "  1. Đăng ký" << std::endl;
        std::cout << "  2. Đăng nhập" << std::endl;
        std::cout << "  3. Thoát" << std::endl;

        std::cout << "> " << std::flush;
        std::string result = InputHandler::waitForInput();

        return result;
    }

    // Handle user input for registration
    std::string displayRegister()
    {
        std::string username;
        std::cout << "\n========= Register =========" << std::endl;
        std::cout << "Username: " << std::flush;

        username = InputHandler::waitForInput();

        return username;
    }

    // Handle user input for login
    std::string displayLogin(NetworkClient &network_client)
    {
        std::string username, password;
        std::cout << "\n========= Login =========" << std::endl;
        std::cout << "Username: " << std::flush;

        username = InputHandler::waitForInput();

        return username;
    }

    // Display the game menu
    std::string displayGameMenu()
    {
        std::cout << "\n========= Game menu =========" << std::endl;
        std::cout << "Chọn hành động: " << std::endl;
        std::cout << "  1. Ghép trận tự động" << std::endl;
        std::cout << "  2. Danh sách người chơi trực tuyến" << std::endl;
        std::cout << "  3. Trở về" << std::endl;

        std::cout << "> " << std::flush;
        std::string result = InputHandler::waitForInput();

        return result;
    }

    // Display the auto match options
    std::string displayAutoMatchOptions()
    {
        std::cout << "\n========= Auto match =========" << std::endl;
        std::cout << "Chọn hành động: " << std::endl;
        std::cout << "  1. Chấp nhận" << std::endl;
        std::cout << "  2. Từ chối" << std::endl;

        std::cout << "> " << std::flush;
        std::string result = InputHandler::waitForInput();

        return result;
    }

    // Display the chess board
    void showBoard(const std::string &fen, bool flip = false)
    {
        board_display::printBoard(fen, flip);
    }

    std::string getMove()
    {
        std::string move;
        std::cout << "Nhập nước đi (VD: e2e4): " << std::flush;

        move = InputHandler::waitForInput();

        return move;
    }

    // Client's decision after seeing the list of players
    struct PlayerListDecision
    {
        int choice;
        std::string username;
    };

    // Display "challenge other players" menu
    PlayerListDecision displayPlayerListOption()
    {
        std::cout << "\n===== Lựa chọn =====" << std::endl;
        std::cout << "1. Thách đấu người chơi khác" << std::endl;
        std::cout << "2. Quay lại" << std::endl;

        std::cout << "> " << std::flush;
        std::string result = InputHandler::waitForInput();
        int choice = std::stoi(result);

        PlayerListDecision decision;

        switch (choice)
        {
            case 1:
                decision.choice = 1;
                std::cout << "Nhập tên người chơi muốn thách đấu: " << std::flush;
                decision.username = InputHandler::waitForInput();
                break;
            default:
                decision.choice = 2;
                break;
        }
        return decision;
    }

    // Display "challenge decision" menu
    std::string displayChallengeDecision()
    {
        std::cout << "\n===== Thách đấu =====" << std::endl;
        std::cout << "1. Chấp nhận" << std::endl;
        std::cout << "2. Từ chối" << std::endl;

        std::cout << "> " << std::flush;
        std::string result = InputHandler::waitForInput(10000);

        return result;
    }

} // namespace UI

#endif // UI_HPP
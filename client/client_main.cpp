#include <iostream>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <csignal>

#include "client_state.hpp"
#include "session_data.hpp"
#include "network_client.hpp"
#include "message_handler.hpp"
#include "input_processor.hpp"
#include "ui.hpp"

// Global state for cleanup
static struct termios g_oldTio;
static int g_stdinFlags = 0;
static ClientState* g_currentState = nullptr;

/**
 * @brief Signal handler for graceful shutdown (Ctrl+C)
 */
void signalHandler(int signum)
{
    if (g_currentState != nullptr)
    {
        *g_currentState = ClientState::EXITING;
    }
}

/**
 * @brief Cleanup terminal settings on exit
 */
void cleanupTerminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &g_oldTio);
    fcntl(STDIN_FILENO, F_SETFL, g_stdinFlags);
}

/**
 * @brief Đọc một dòng từ stdin (non-blocking)
 */
bool readLineNonBlocking(std::string &line_buffer)
{
    char c;
    while (read(STDIN_FILENO, &c, 1) > 0)
    {
        if (c == '\n' || c == '\r')
        {
            std::cout << std::endl;
            return true;
        }
        else if (c == 127 || c == 8)  // Backspace
        {
            if (!line_buffer.empty())
            {
                line_buffer.pop_back();
                std::cout << "\b \b" << std::flush;
            }
        }
        else if (c >= 32 && c < 127)  // Printable
        {
            line_buffer += c;
            std::cout << c << std::flush;
        }
    }
    return false;
}

int main()
{
    NetworkClient &network = NetworkClient::getInstance();
    MessageHandler messageHandler;
    InputProcessor inputProcessor;

    std::signal(SIGINT, signalHandler);

    // Save and setup terminal
    g_stdinFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    tcgetattr(STDIN_FILENO, &g_oldTio);
    std::atexit(cleanupTerminal);

    fcntl(STDIN_FILENO, F_SETFL, g_stdinFlags | O_NONBLOCK);

    struct termios new_tio = g_oldTio;
    new_tio.c_lflag &= ~(ICANON | ECHO);
    new_tio.c_cc[VMIN] = 0;
    new_tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    // State machine
    ClientState currentState = ClientState::INITIAL_MENU;
    g_currentState = &currentState;
    StateContext context;
    std::string inputBuffer;

    UI::clearConsole();
    UI::printLogo();
    UI::displayInitialMenuPrompt();

    int socket_fd = network.getSocketFd();

    // Setup poll
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = socket_fd;
    fds[1].events = POLLIN;

    // Main event loop
    while (currentState != ClientState::EXITING)
    {
        int ret = poll(fds, 2, 10);  // 10ms timeout for very responsive input

        if (ret < 0)
        {
            if (errno == EINTR) continue;
            break;
        }

        // Process stdin
        if (fds[0].revents & POLLIN)
        {
            if (readLineNonBlocking(inputBuffer))
            {
                currentState = inputProcessor.processInput(currentState, inputBuffer, context);
                inputBuffer.clear();
            }
        }

        // Process ALL available packets from server
        if (fds[1].revents & POLLIN)
        {
            Packet packet;
            while (network.receivePacket(packet))
            {
                ClientState newState = messageHandler.handleMessage(currentState, packet, context);
                if (newState != currentState)
                {
                    inputBuffer.clear();
                }
                currentState = newState;
                
                if (currentState == ClientState::EXITING) break;
            }
            
            // Check if connection was closed
            if (fds[1].revents & (POLLHUP | POLLERR))
            {
                UI::printErrorMessage("Mất kết nối đến server.");
                currentState = ClientState::EXITING;
            }
        }
    }

    std::cout << "\nClient đã đóng kết nối." << std::endl;
    return 0;
}

#include <iostream>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <csignal>

#include "client_state.hpp"
#include "session_data.hpp"
#include "network_client.hpp"
#include "message_handler.hpp"
#include "input_processor.hpp"
#include "ui.hpp"

// Global state for signal handling
static ClientState* g_currentState = nullptr;
static struct termios g_oldTio;
static int g_stdinFlags = 0;

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
 * @brief Đọc một dòng từ stdin (non-blocking với buffer)
 * 
 * @param line_buffer Buffer tích lũy ký tự
 * @return true nếu có dòng hoàn chỉnh (kết thúc bằng newline)
 */
bool readLineNonBlocking(std::string &line_buffer)
{
    char c;
    while (read(STDIN_FILENO, &c, 1) > 0)
    {
        if (c == '\n' || c == '\r')
        {
            std::cout << std::endl;  // Echo newline
            return true;  // Complete line
        }
        else if (c == 127 || c == 8)  // Backspace (127) or Delete (8)
        {
            if (!line_buffer.empty())
            {
                line_buffer.pop_back();
                // Erase character on screen: move back, print space, move back again
                std::cout << "\b \b" << std::flush;
            }
        }
        else if (c >= 32 && c < 127)  // Printable ASCII characters only
        {
            line_buffer += c;
            std::cout << c << std::flush;  // Echo character
        }
        // Ignore other control characters (arrows, etc.)
    }
    return false;  // No complete line yet
}

/**
 * @brief Main event loop sử dụng select()
 */
int main()
{
    // Initialize components
    NetworkClient &network = NetworkClient::getInstance();
    MessageHandler messageHandler;
    InputProcessor inputProcessor;

    // Setup signal handler for Ctrl+C
    std::signal(SIGINT, signalHandler);

    // Save original terminal settings
    g_stdinFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    tcgetattr(STDIN_FILENO, &g_oldTio);
    
    // Register cleanup on exit
    std::atexit(cleanupTerminal);

    // Set stdin to non-blocking mode
    fcntl(STDIN_FILENO, F_SETFL, g_stdinFlags | O_NONBLOCK);

    // Disable canonical mode and echo for stdin
    struct termios new_tio = g_oldTio;
    new_tio.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode AND echo
    new_tio.c_cc[VMIN] = 0;               // Don't wait for characters
    new_tio.c_cc[VTIME] = 0;              // No timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    // Initialize state
    ClientState currentState = ClientState::INITIAL_MENU;
    g_currentState = &currentState;  // For signal handler
    StateContext context;
    std::string inputBuffer;

    // Display initial prompt
    UI::clearConsole();
    UI::printLogo();
    UI::displayInitialMenuPrompt();

    int socket_fd = network.getSocketFd();
    int max_fd = (socket_fd > STDIN_FILENO) ? socket_fd : STDIN_FILENO;

    // Main event loop - controlled by state machine only
    while (currentState != ClientState::EXITING)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(socket_fd, &read_fds);

        // Timeout for select (100ms)
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100ms

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0)
        {
            if (errno == EINTR)
            {
                // Interrupted by signal (e.g., Ctrl+C)
                continue;
            }
            perror("select error");
            currentState = ClientState::EXITING;
            break;
        }

        // Check for user input
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            if (readLineNonBlocking(inputBuffer))
            {
                currentState = inputProcessor.processInput(currentState, inputBuffer, context);
                inputBuffer.clear();
            }
        }

        // Check for server message - process ALL buffered packets
        if (FD_ISSET(socket_fd, &read_fds))
        {
            Packet packet;
            // Keep processing while there are complete packets in buffer
            while (network.receivePacket(packet))
            {
                ClientState newState = messageHandler.handleMessage(currentState, packet, context);
                
                // Clear input buffer when state changes from server message
                if (newState != currentState)
                {
                    inputBuffer.clear();
                }
                
                currentState = newState;
                
                if (currentState == ClientState::EXITING)
                {
                    break;
                }
            }
        }
    }

    // Cleanup is handled by atexit()
    std::cout << "\nClient đã đóng kết nối." << std::endl;
    return 0;
}
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <atomic>
#include <sstream>

// Tái tạo MessageType và Packet từ protocol.hpp
enum class MessageType : uint8_t
{
    LOGIN_SUCCESS = 0x21,
    CHALLENGE_NOTIFICATION = 0x51,
    GAME_START = 0x40
};

std::ostream& operator<<(std::ostream& os, const MessageType& type)
{
    switch (type)
    {
    case MessageType::LOGIN_SUCCESS:
        os << "LOGIN_SUCCESS";
        break;
    case MessageType::CHALLENGE_NOTIFICATION:
        os << "CHALLENGE_NOTIFICATION";
        break;
    case MessageType::GAME_START:
        os << "GAME_START";
        break;
    default:
        os << "UNKNOWN";
        break;
    }
    return os;
}

struct Packet
{
    MessageType type;
    std::vector<uint8_t> payload;
};

class InputHandler {
private:
    static std::pair<std::string, std::string> current_input; // {context, value}
    static std::mutex input_mutex;
    static std::condition_variable input_cv;
    static std::string current_context;
    static std::mutex context_mutex;
    
    // Thêm callback để kiểm tra điều kiện hủy
    static std::function<bool()> cancel_check;
    static std::mutex cancel_mutex;

public:
    static void setContext(const std::string& context) {
        std::lock_guard<std::mutex> lock(context_mutex);
        std::cout << "\n[InputHandler] Context changed to: " << context << std::endl;
        current_context = context;
    }

    static std::string getContext() {
        std::lock_guard<std::mutex> lock(context_mutex);
        return current_context;
    }

    static void setCancelCheck(std::function<bool()> check) {
        std::lock_guard<std::mutex> lock(cancel_mutex);
        cancel_check = check;
    }

    static void startInputThread(std::atomic<bool>& running) {
        std::thread([&running]() {
            std::cout << "\n[InputHandler] Input thread started" << std::endl;
            
            while (running) {
                std::string input;
                std::getline(std::cin, input);
                
                {
                    std::lock_guard<std::mutex> lock(input_mutex);
                    current_input = {getContext(), input};
                    std::cout << "\n[InputHandler] Received input '" << input 
                              << "' for context: " << current_input.first << std::endl;
                }
                
                input_cv.notify_all();
            }
            
            std::cout << "\n[InputHandler] Input thread stopped" << std::endl;
        }).detach();
    }

    static std::string waitForInput(const std::string& context, int timeout_ms = -1) {
        std::unique_lock<std::mutex> lock(input_mutex);
        
        auto predicate = [&context]() {
            std::lock_guard<std::mutex> cancel_lock(cancel_mutex);
            // Trả về true nếu hoặc là có input phù hợp HOẶC cancel_check return true
            return (current_input.first == context) || 
                   (cancel_check && cancel_check());
        };

        bool got_input = false;
        if (timeout_ms > 0) {
            got_input = input_cv.wait_for(lock, 
                std::chrono::milliseconds(timeout_ms), 
                predicate);
        } else {
            input_cv.wait(lock, predicate);
            got_input = true;
        }

        // Kiểm tra xem có phải bị hủy không
        {
            std::lock_guard<std::mutex> cancel_lock(cancel_mutex);
            if (cancel_check && cancel_check()) {
                return "";
            }
        }

        if (got_input && current_input.first == context) {
            std::string value = current_input.second;
            current_input = {"", ""};
            return value;
        }
        return "";
    }
};

// Khởi tạo static members
std::pair<std::string, std::string> InputHandler::current_input;
std::mutex InputHandler::input_mutex;
std::condition_variable InputHandler::input_cv;
std::string InputHandler::current_context = "initial";
std::mutex InputHandler::context_mutex;
std::function<bool()> InputHandler::cancel_check;
std::mutex InputHandler::cancel_mutex;

// Mock Server để giả lập gửi packet
class MockServer
{
public:
    static void sendPacket(std::function<void(Packet)> handler, int delay_ms)
    {
        std::thread([handler, delay_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            // Giả lập challenge notification sau khi login
            Packet challenge_packet;
            challenge_packet.type = MessageType::CHALLENGE_NOTIFICATION;
            handler(challenge_packet); 
        }).detach();
    }
};

// Message Handler
class MessageHandler {
private:
    std::atomic<std::thread::id> current_handler_id;
    std::mutex handler_mutex;

    bool isCurrentHandler() {
        return std::this_thread::get_id() == current_handler_id.load();
    }

    void setCurrentHandler(std::thread::id id) {
        current_handler_id.store(id);
        
        // Cập nhật cancel check cho InputHandler
        InputHandler::setCancelCheck([this]() {
            return !isCurrentHandler();
        });
    }

    bool shouldStop() {
        return !isCurrentHandler();
    }

public:
    MessageHandler() : current_handler_id(std::thread::id()) {}

    void pushMessage(const Packet& packet) {
        std::thread([this, packet]() {
            {
                std::lock_guard<std::mutex> lock(handler_mutex);
                setCurrentHandler(std::this_thread::get_id());
            }

            std::cout << "\n[MessageHandler] Starting to handle " << packet.type << std::endl;
            handleMessage(packet);
            
            if (isCurrentHandler()) {
                setCurrentHandler(std::thread::id());
            }
        }).detach();
    }

private:
    void handleMessage(const Packet &packet) {
        switch (packet.type) {
        case MessageType::LOGIN_SUCCESS:
            handleLoginSuccess();
            break;
        case MessageType::CHALLENGE_NOTIFICATION:
            handleChallengeNotification();
            break;
        case MessageType::GAME_START:
            handleGameStart();
            break;
        }
    }

    void handleLoginSuccess() {
        // if (shouldStop()) return;
        
        std::cout << "\n[MessageHandler] Login success, displaying game menu..." << std::endl;
        InputHandler::setContext("game_menu");
        displayGameMenu();
    }

    void handleChallengeNotification() {
        // if (shouldStop()) return;

        std::cout << "\n[MessageHandler] Received challenge notification!" << std::endl;
        InputHandler::setContext("challenge_response");

        std::cout << "=== Challenge Notification ===" << std::endl;
        std::cout << "Player 'challenger' wants to play with you!" << std::endl;
        std::cout << "Accept? (y/n) > " << std::flush;

        std::string response = InputHandler::waitForInput("challenge_response", 5000);
        
        // if (shouldStop()) return;

        if (response.empty()) {
            std::cout << "\n[MessageHandler] Challenge response timeout or cancelled" << std::endl;
        } else {
            std::cout << "\n[MessageHandler] Challenge response: " << response << std::endl;
        }
    }

    void handleGameStart() {
        // if (shouldStop()) return;
        std::cout << "\n[MessageHandler] Game starting..." << std::endl;
    }

    void displayGameMenu() {
        // if (shouldStop()) return;

        std::cout << "\n=== Game Menu ===" << std::endl;
        std::cout << "1. Auto Match" << std::endl;
        std::cout << "2. Player List" << std::endl;
        std::cout << "3. Exit" << std::endl;
        std::cout << "> " << std::flush;

        std::string choice = InputHandler::waitForInput("game_menu");
        std::cout << "\n[MessageHandler] Game menu" << choice << std::endl;
        if (shouldStop()) return;
        
        std::cout << "\n[MessageHandler] Game menu choice: " << choice << std::endl;
    }
};

int main() {
    std::atomic<bool> running{true};
    MessageHandler message_handler;

    // Start input thread
    InputHandler::startInputThread(running);

    // Simulate initial login success
    Packet login_success_packet;
    login_success_packet.type = MessageType::LOGIN_SUCCESS;
    message_handler.pushMessage(login_success_packet);

    // Mock server sẽ gửi challenge notification sau 3 giây
    MockServer::sendPacket([&message_handler](Packet p) {
        std::cout << "\n[MockServer] Sending challenge notification..." << std::endl;
        message_handler.pushMessage(p);
    }, 3000);

    // Main thread waits
    std::this_thread::sleep_for(std::chrono::seconds(10));
    running = false;

    return 0;
}
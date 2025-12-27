#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <thread>
#include <iostream>

class InputHandler {
private:
    static std::string current_input;
    static bool has_new_input;
    static std::mutex input_mutex;
    static std::condition_variable input_cv;
    static std::function<bool()> cancel_check;
    static std::mutex cancel_mutex;

public:
    static void setCancelCheck(std::function<bool()> check) {
        std::lock_guard<std::mutex> lock(cancel_mutex);
        cancel_check = check;
    }

    static void startInputThread(std::atomic<bool>& running) {
        std::thread([&running]() {
            // std::cout << "\n[InputHandler] Input thread started" << std::endl;
            
            while (running) {
                std::string input;
                std::getline(std::cin, input);
                
                {
                    std::lock_guard<std::mutex> lock(input_mutex);
                    current_input = input;
                    has_new_input = true;
                }
                
                input_cv.notify_all();
            }
            
            // std::cout << "\n[InputHandler] Input thread stopped" << std::endl;
        }).detach();
    }

    static std::string waitForInput(int timeout_ms = -1) {
        std::unique_lock<std::mutex> lock(input_mutex);
        
        auto predicate = []() {
            std::lock_guard<std::mutex> cancel_lock(cancel_mutex);
            return has_new_input || (cancel_check && cancel_check());
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

        {
            std::lock_guard<std::mutex> cancel_lock(cancel_mutex);
            if (cancel_check && cancel_check()) {
                return "<CANCELLED>";
            }
        }

        if (got_input && has_new_input) {
            has_new_input = false;
            return current_input;
        }
        return "<TIMEOUT>";
    }
};

// Static member definitions
std::string InputHandler::current_input;
bool InputHandler::has_new_input = false;
std::mutex InputHandler::input_mutex;
std::condition_variable InputHandler::input_cv;
std::function<bool()> InputHandler::cancel_check;
std::mutex InputHandler::cancel_mutex;

#endif // INPUT_HANDLER_HPP
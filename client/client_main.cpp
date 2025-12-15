#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

#include "network_client.hpp"
#include "message_handler.hpp"
#include "logic_handler.hpp"
#include "input_handler.hpp"

int main() {
    NetworkClient &network_client = NetworkClient::getInstance();
    SessionData &session_data = SessionData::getInstance();
    session_data.setRunning(true);

    // Start input thread
    InputHandler::startInputThread(session_data.getRunningAtomic());

    // Initial menu handling in a separate thread
    std::thread menu_thread([&]() {
        session_data.setCurrentHandler(std::this_thread::get_id());
        LogicHandler logic_handler;
        logic_handler.handleInitialMenu();
    });
    menu_thread.detach();

    MessageHandler handler;

    while (session_data.getRunning()) {
        Packet packet;
        bool received = network_client.receivePacket(packet);
        if (received) {
            bool isSuccess = handler.pushMessage(packet);
            if (!isSuccess) {
                std::cout << "Xử lý thông điệp thất bại." << std::endl;
                session_data.setRunning(false);
            }
        }
    }

    std::cout << "Client đã đóng kết nối." << std::endl;
    return 0;
}
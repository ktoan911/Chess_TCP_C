#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "../server/network_server.hpp"
#include "../server/data_storage.hpp"
#include "../server/game_manager.hpp"
#include "../client/network_client.hpp"
#include "../server/message_handler.hpp"

// Hàm chạy server
void runServer() {
    NetworkServer &server = NetworkServer::getInstance();
    DataStorage &storage = DataStorage::getInstance();
    GameManager &gameManager = GameManager::getInstance();
    MessageHandler handler(server, storage, gameManager);

    std::cout << "Server bắt đầu lắng nghe..." << std::endl;

    while (true) {
        int client_fd = server.acceptConnection();
        if (client_fd < 0) continue;

        // Xử lý mỗi kết nối khách hàng trong một luồng riêng
        std::thread([client_fd, &handler, &server]() {
            while (true) {
                Packet packet;
                if (server.receivePacket(client_fd, packet)) {
                    handler.handleMessage(client_fd, packet);
                } else {
                    std::cout << "Kết nối với client fd " << client_fd << " đã bị đóng." << std::endl;
                    server.removeUsername(client_fd);
                    server.closeConnection(client_fd);
                    break;
                }
            }
        }).detach();
    }
}

std::string currentUsername = "hnv1420";

// Hàm chạy client
void runClient(int client_id) {
    NetworkClient &client = NetworkClient::getInstance();

    // Tạo message Login
    LoginMessage loginMsg;
    loginMsg.username = currentUsername;
    std::vector<uint8_t> serialized = loginMsg.serialize();

    if (currentUsername == "hnv1420") {
        currentUsername = "hnv22"; // Đổi username để test trường hợp đăng nhập cùng username
    }

    std::cout << "Client " << client_id << " gửi yêu cầu đăng nhập với username: " << loginMsg.username << std::endl;
    // std::cout << "> " << std::endl;
    // std::cin.get();

    // Gửi packet Login
    if (client.sendPacket(MessageType::LOGIN, serialized)) {
        std::cout << "Client " << client_id << " đã gửi yêu cầu đăng nhập thành công." << std::endl;
    } else {
        std::cout << "Client " << client_id << " gửi yêu cầu đăng nhập thất bại." << std::endl;
        return;
    }

    // Nhận phản hồi
    Packet response;
    while (true) {
        if (client.receivePacket(response)) {
            switch (response.type) {
                case MessageType::LOGIN_SUCCESS: {
                    LoginSuccessMessage success = LoginSuccessMessage::deserialize(response.payload);
                    std::cout << "Client " << client_id << " đăng nhập thành công với ELO: " << success.elo << std::endl;
                    break;
                }
                case MessageType::LOGIN_FAILURE: {
                    LoginFailureMessage failure = LoginFailureMessage::deserialize(response.payload);
                    std::cout << "Client " << client_id << " đăng nhập thất bại: " << failure.error_message << std::endl;
                    break;
                }
                default:
                    std::cout << "Client " << client_id << "nhận được loại message không xác định." << std::endl;
                    break;
            }
            break;
        }
        // Chờ một chút trước khi thử lại
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    // Bước 1: Khởi động server trong một luồng riêng
    std::thread serverThread(runServer);
    // Đợi server khởi động
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Bước 2: Khởi động nhiều client cố gắng đăng nhập cùng một username
    const int NUM_CLIENTS = 3; // Số lượng client mô phỏng
    std::vector<std::thread> clientThreads;

    for (int i = 1; i <= NUM_CLIENTS; ++i) {
        clientThreads.emplace_back(runClient, i);
        // Giãn một chút giữa các client để dễ quan sát
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Bước 3: Chờ tất cả client hoàn thành
    for (auto &t : clientThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Dừng server (trong thực tế cần có cơ chế dừng mềm mại)
    // Ở đây chỉ để ví dụ nên chương trình kết thúc
    std::cout << "Tất cả client đã hoàn thành." << std::endl;
    return 0;
}
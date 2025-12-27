#include <thread>
#include <vector>
#include <iostream>
#include <csignal>

#include "network_server.hpp"
#include "message_handler.hpp"

#include "../common/message.hpp"
#include "../common/const.hpp"

void handleClient(int client_fd);

int main()
{
    // Khởi tạo các singletons
    NetworkServer &network_server = NetworkServer::getInstance();
    DataStorage &data_storage = DataStorage::getInstance();
    GameManager &game_manager = GameManager::getInstance();

    // Khởi tạo GameManager với dependencies (DI)
    game_manager.init(network_server, data_storage);

    // Tạo một vector chứa tất cả các thread xử lý client
    std::vector<std::thread> client_threads;

    while (true)
    {
        int client_fd = network_server.acceptConnection();
        if (client_fd != -1)
        {
            // Tạo một thread mới để xử lý client
            client_threads.emplace_back(std::thread(handleClient, client_fd));
        }
    }

    // Join tất cả các thread trước khi kết thúc chương trình
    for (auto &th : client_threads)
    {
        if (th.joinable())
        {
            th.join();
        }
    }

    network_server.closeAllConnections();

    return 0;
}

void handleClient(int client_fd)
{
    NetworkServer &network_server = NetworkServer::getInstance();
    DataStorage &storage = DataStorage::getInstance();
    GameManager &game_manager = GameManager::getInstance();
    MessageHandler message_handler(network_server, storage, game_manager);

    while (true)
    {
        Packet packet;
        bool received = network_server.receivePacket(client_fd, packet);
        if (!received)
        {
            std::cout << "Client " << client_fd << " ngắt kết nối." << std::endl;
            game_manager.clientDisconnected(client_fd);

            network_server.closeConnection(client_fd);
            break;
        }

        // Handle message
        message_handler.handleMessage(client_fd, packet);
    }
}
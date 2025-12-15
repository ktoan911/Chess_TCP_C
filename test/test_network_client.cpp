#include <thread>
#include <chrono>

#include "../client/network_client.hpp"
#include "../server/network_server.hpp"

// Simple server function for testing
void serverFunction(uint16_t port) {
    std::cout << ">> Starting NetworkServer..." << std::endl;
    // Initialize NetworkServer instance
    NetworkServer &server = NetworkServer::getInstance();

    // Accept client connection
    int client_fd = server.acceptConnection();
    if (client_fd == -1) {
        std::cout << ">> Server failed to accept client connection." << std::endl;
        return;
    }
    std::cout << ">> Server accepted client connection." << std::endl;

    // Receive packet from client
    Packet receivedPacket;
    if (server.receivePacket(client_fd, receivedPacket)) {
        std::cout << ">> Server received packet: "
                  << "Type=0x" << std::hex << static_cast<int>(receivedPacket.type)
                  << ", Length=" << receivedPacket.length
                  << ", Payload=" << std::string(receivedPacket.payload.begin(), receivedPacket.payload.end()) << std::endl;
    } else {
        std::cout << ">> Server failed to receive packet." << std::endl;
    }

    // Send response packet to client
    std::vector<uint8_t> payload = {'O', 'K'};
    if (server.sendPacket(client_fd, MessageType::RESPONSE, payload)) {
        std::cout << ">> Server sent response packet." << std::endl;
    } else {
        std::cout << ">> Server failed to send response packet." << std::endl;
    }

    // Close connection
    server.closeConnection(client_fd);
    std::cout << ">> Server closed client connection." << std::endl;
}

int main() {
    // Start server thread
    std::thread serverThread(serverFunction, Const::SERVER_PORT);

    std::cout << ">> Starting NetworkClient..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for server to be ready

    // Initialize NetworkClient instance
    NetworkClient &client = NetworkClient::getInstance();

    // Send packet to server
    std::vector<uint8_t> payload = {'H', 'e', 'l', 'l', 'o'};
    if (client.sendPacket(MessageType::TEST, payload)) {
        std::cout << ">> Client sent test packet." << std::endl;
    } else {
        std::cout << ">> Client failed to send test packet." << std::endl;
    }

    // Receive response packet from server
    Packet receivedPacket;
    if (client.receivePacket(receivedPacket)) {
        std::cout << ">> Client received packet: "
                  << "Type=0x" << std::hex << static_cast<int>(receivedPacket.type)
                  << ", Length=" << receivedPacket.length
                  << ", Payload=" << std::string(receivedPacket.payload.begin(), receivedPacket.payload.end()) << std::endl;
    } else {
        std::cout << ">> Client failed to receive packet." << std::endl;
    }

    // Close client connection
    client.closeConnection();
    std::cout << ">> Client closed connection." << std::endl;

    serverThread.join();
    std::cout << ">> Test completed." << std::endl;

    return 0;
}
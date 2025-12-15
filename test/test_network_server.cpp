#include "../server/network_server.hpp"
#include <thread>
#include <chrono>

// Simple client function for testing
void clientFunction(uint16_t port)
{
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for server to be ready

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("Client socket creation failed");
        return;
    }

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    const char *ip = Const::SERVER_IP.c_str();

    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Client connection failed");
        close(sock);
        return;
    }
    std::cout << ">> Client connected to server." << std::endl;

    // Receive packet from server
    Packet packet;
    NetworkServer::getInstance().receivePacket(sock, packet);
    std::cout << ">> Client received packet: "
              << "Type=0x" << std::hex << static_cast<int>(packet.type)
              << ", Length=" << packet.length
              << ", Payload=" << std::string(packet.payload.begin(), packet.payload.end()) << std::endl;

    // Send response packet to server
    std::vector<uint8_t> payload = {'H', 'i'};
    NetworkServer::getInstance().sendPacket(sock, MessageType::RESPONSE, payload);
    std::cout << ">> Client sent response packet." << std::endl;

    close(sock);
}

int main()
{
    std::cout << ">> Starting NetworkServer..." << std::endl;
    NetworkServer &server = NetworkServer::getInstance();

    // Start client thread
    std::thread clientThread(clientFunction, Const::SERVER_PORT);

    // Accept client connection
    int client_fd = server.acceptConnection();
    if (client_fd == -1)
    {
        std::cout << ">> Failed to accept client connection." << std::endl;
        clientThread.join();
        return 1;
    }
    std::cout << ">> Server accepted client connection." << std::endl;

    // Send packet to client
    std::vector<uint8_t> payload = {'T', 'e', 's', 't'};
    if (server.sendPacket(client_fd, MessageType::TEST, payload))
    {
        std::cout << ">> Server sent test packet." << std::endl;
    }
    else
    {
        std::cout << ">> Server failed to send test packet." << std::endl;
    }

    // Receive response packet from client
    Packet receivedPacket;
    if (server.receivePacket(client_fd, receivedPacket))
    {
        std::cout << ">> Server received packet: "
                  << "Type=0x" << std::hex << static_cast<int>(receivedPacket.type)
                  << ", Length=" << receivedPacket.length
                  << ", Payload=" << std::string(receivedPacket.payload.begin(), receivedPacket.payload.end()) << std::endl;
    }
    else
    {
        std::cout << ">> Server failed to receive packet." << std::endl;
    }

    // Close connections
    server.closeConnection(client_fd);
    std::cout << ">> Server closed client connection." << std::endl;

    clientThread.join();
    std::cout << ">> Test completed." << std::endl;

    return 0;
}
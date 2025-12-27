// test_message_handler.cpp

#include "message_handler.hpp"
#include "network_server.hpp"
#include "data_storage.hpp"
#include "game_manager.hpp"
#include "../common/protocol.hpp"
#include "../common/message.hpp"
#include <iostream>
#include <vector>

int main() {
    NetworkServer &server = NetworkServer::getInstance();
    DataStorage &storage = DataStorage::getInstance();
    GameManager &gameManager = GameManager::getInstance();
    MessageHandler handler(server, storage, gameManager);

    // Test REGISTER message
    RegisterMessage register_msg;
    register_msg.username = "test_user";
    std::vector<uint8_t> register_payload = register_msg.serialize();
    Packet register_packet;
    register_packet.type = MessageType::REGISTER;
    register_packet.payload = register_payload;
    std::cout << "Testing REGISTER message..." << std::endl;
    handler.handleMessage(1, register_packet);
    std::cout << "REGISTER test completed.\n" << std::endl;

    // Test LOGIN message
    LoginMessage login_msg;
    login_msg.username = "test_user";
    std::vector<uint8_t> login_payload = login_msg.serialize();
    Packet login_packet;
    login_packet.type = MessageType::LOGIN;
    login_packet.payload = login_payload;
    std::cout << "Testing LOGIN message..." << std::endl;
    handler.handleMessage(2, login_packet);
    std::cout << "LOGIN test completed.\n" << std::endl;

    // Test MOVE message
    MoveMessage move_msg;
    move_msg.game_id = "game123";
    move_msg.uci_move = "e2e4";
    
    std::vector<uint8_t> move_payload = move_msg.serialize();
    Packet move_packet;
    move_packet.type = MessageType::MOVE;
    move_packet.payload = move_payload;
    std::cout << "Testing MOVE message..." << std::endl;
    handler.handleMessage(3, move_packet);
    std::cout << "MOVE test completed.\n" << std::endl;

    // Test UNKNOWN message
    std::vector<uint8_t> unknown_payload = {}; // Empty payload for unknown
    Packet unknown_packet;
    unknown_packet.type = static_cast<MessageType>(999);
    unknown_packet.payload = unknown_payload;
    std::cout << "Testing UNKNOWN message..." << std::endl;
    handler.handleMessage(4, unknown_packet);
    std::cout << "UNKNOWN test completed.\n" << std::endl;

    return 0;
}
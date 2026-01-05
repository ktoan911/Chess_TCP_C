#ifndef NETWORK_SERVER_HPP
#define NETWORK_SERVER_HPP

// Thư viện chuẩn
#include <cstring>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <vector>

// Thư viện socket (Linux)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Header tự định nghĩa
#include "../common/const.hpp"
#include "../common/message.hpp"
#include "../common/protocol.hpp"
#include "structs.hpp"

/**
 * @brief Lớp NetworkServer (Singleton) - Quản lý kết nối mạng của server.
 * Chịu trách nhiệm khởi tạo socket, chấp nhận kết nối, và gửi/nhận dữ liệu với
 * clients.
 */
class NetworkServer {
private:
  int server_fd; // File descriptor của server socket
  std::unordered_map<int, ClientInfo>
      clients;              // Map quản lý thông tin clients (Key: fd)
  std::mutex clients_mutex; // Mutex bảo vệ truy cập vào map clients

  /**
   * @brief Khởi tạo server socket, bind địa chỉ và bắt đầu lắng nghe.
   * @param port Cổng server lắng nghe.
   */
  void initialize(uint16_t port) {
    // 1. Tạo socket TCP/IPv4
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
      perror("socket failed");
      exit(EXIT_FAILURE);
    }

    // 2. Thiết lập địa chỉ server
    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Lắng nghe trên mọi interface
    address.sin_port = htons(port);

    // 3. Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      perror("bind failed");
      close(server_fd);
      exit(EXIT_FAILURE);
    }

    // 4. Lắng nghe kết nối
    if (listen(server_fd, Const::BACKLOG) < 0) {
      perror("listen failed");
      close(server_fd);
      exit(EXIT_FAILURE);
    }

    std::cout << "Server đang lắng nghe trên: " << inet_ntoa(address.sin_addr)
              << ":" << ntohs(address.sin_port) << " ..." << std::endl;
  }

  // Constructor private (Singleton)
  NetworkServer() : server_fd(-1) { initialize(Const::SERVER_PORT); }

public:
  // Ngăn copy/assignment để đảm bảo tính duy nhất của Singleton
  NetworkServer(const NetworkServer &) = delete;
  NetworkServer &operator=(const NetworkServer &) = delete;

  // Destructor: Đóng socket khi hủy
  ~NetworkServer() {
    if (server_fd != -1)
      close(server_fd);
  }

  /**
   * @brief Lấy instance duy nhất của NetworkServer (Singleton).
   */
  static NetworkServer &getInstance() {
    static NetworkServer instance;
    return instance;
  }

  /**
   * @brief Chấp nhận kết nối mới từ client (Blocking).
   * @return File descriptor của client, hoặc -1 nếu lỗi.
   */
  int acceptConnection() {
    sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);

    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_address, &client_len);
    if (client_fd < 0) {
      perror("accept failed");
      return -1;
    }

    std::cout << "Client kết nối từ: " << inet_ntoa(client_address.sin_addr)
              << ":" << ntohs(client_address.sin_port) << " (fd = " << client_fd
              << ")" << std::endl;
    return client_fd;
  }

  /**
   * @brief Gửi gói tin đến client qua file descriptor.
   * Serialize packet và gửi qua socket.
   */
  bool sendPacket(int client_fd, MessageType messageType,
                  const std::vector<uint8_t> &payload) {
    Packet packet;
    packet.type = messageType;
    packet.length = htons(static_cast<uint16_t>(payload.size()));
    packet.payload = payload;

    std::vector<uint8_t> serialized = packet.serialize();

    ssize_t sent = send(client_fd, serialized.data(), serialized.size(), 0);
    if (sent != static_cast<ssize_t>(serialized.size())) {
      perror("send failed");
      return false;
    }
    return true;
  }

  /**
   * @brief Gửi gói tin đến client qua username.
   */
  bool sendPacketToUsername(const std::string &username,
                            MessageType messageType,
                            const std::vector<uint8_t> &payload) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto &pair : clients) {
      if (pair.second.username == username) {
        return sendPacket(pair.first, messageType, payload);
      }
    }
    std::cerr << "Username " << username << " không tìm thấy." << std::endl;
    return false;
  }

  /**
   * @brief Nhận gói tin từ client.
   * Xử lý TCP stream: đọc dữ liệu vào buffer, ghép packet (Header + Payload) và
   * trả về khi đủ.
   * @return true nếu nhận đủ 1 packet, false nếu chưa đủ hoặc lỗi.
   */
  bool receivePacket(int client_fd, Packet &packet) {
    // 1. Nhận dữ liệu từ socket
    uint8_t buffer_temp[Const::BUFFER_SIZE];
    ssize_t bytes_received =
        recv(client_fd, buffer_temp, sizeof(buffer_temp), 0);
    if (bytes_received <= 0)
      return false;

    // 2. Thêm vào buffer của client
    {
      std::lock_guard<std::mutex> lock(clients_mutex);
      clients[client_fd].buffer.insert(clients[client_fd].buffer.end(),
                                       buffer_temp,
                                       buffer_temp + bytes_received);
    }

    // 3. Xử lý buffer để tách packet
    {
      std::lock_guard<std::mutex> lock(clients[client_fd].mutex);
      auto &buffer = clients[client_fd].buffer;

      while (buffer.size() >= 3) { // Header = 1 byte Type + 2 bytes Length
        MessageType type = static_cast<MessageType>(buffer[0]);
        uint16_t length = (static_cast<uint16_t>(buffer[1]) << 8) |
                          static_cast<uint16_t>(buffer[2]);
        length = ntohs(length);

        if (buffer.size() < 3 + length)
          break; // Chưa đủ dữ liệu

        // Trích xuất payload
        std::vector<uint8_t> payload(buffer.begin() + 3,
                                     buffer.begin() + 3 + length);
        packet = Packet{type, length, payload};

        // Xóa packet đã xử lý khỏi buffer
        buffer.erase(buffer.begin(), buffer.begin() + 3 + length);
        return true;
      }
    }
    return false;
  }

  // ===== CÁC PHƯƠNG THỨC QUẢN LÝ CLIENT & UTILS =====

  void setUsername(int client_fd, const std::string &username) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients[client_fd].username = username;
  }

  std::string getUsername(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    return clients[client_fd].username;
  }

  int getClientFD(const std::string &username) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto &pair : clients) {
      if (pair.second.username == username)
        return pair.first;
    }
    return -1;
  }

  std::string getClientIP(int client_fd) {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(client_fd, (struct sockaddr *)&addr, &addr_len) == 0) {
      return std::string(inet_ntoa(addr.sin_addr));
    }
    return "";
  }

  std::string getClientIPByUsername(const std::string &username) {
    int client_fd = getClientFD(username);
    return (client_fd != -1) ? getClientIP(client_fd) : "";
  }

  bool isUserLoggedIn(const std::string &username) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto &pair : clients) {
      if (pair.second.username == username)
        return true;
    }
    return false;
  }

  bool isClientConnected(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    return clients.find(client_fd) != clients.end();
  }

  void closeConnection(int client_fd) {
    close(client_fd);
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(client_fd);
  }

  void closeAllConnections() {
    std::lock_guard<std::mutex> lock(clients_mutex);
    close(server_fd);
    for (auto &pair : clients) {
      close(pair.first);
    }
    clients.clear();
  }

  void dispose() {
    closeAllConnections();
    // Lưu ý: Không delete Singleton instance
  }
}; // Kết thúc class NetworkServer

// Kết thúc header guard - đóng #ifndef NETWORK_SERVER_HPP ở đầu file
#endif // NETWORK_SERVER_HPP
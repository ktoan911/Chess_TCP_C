#ifndef NETWORK_CLIENT_HPP
#define NETWORK_CLIENT_HPP

#include <string>
#include <vector>
#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#include "../common/protocol.hpp"
#include "../common/message.hpp"
#include "../common/utils.hpp"
#include "../common/const.hpp"

#include "session_data.hpp"

/**
 * @brief Singleton quản lý kết nối mạng TCP với máy chủ.
 *
 * Lớp NetworkClient là một singleton dùng để thiết lập và duy trì kết nối TCP với máy chủ.
 * Nó cung cấp các phương thức để gửi và nhận gói tin, đảm bảo tính thread-safe khi gửi dữ liệu.
 */
class NetworkClient
{
private:
    int socket_fd;
    std::vector<uint8_t> buffer;
    std::mutex send_mutex;

    /**
     * @brief Kết nối đến máy chủ với IP và cổng được cung cấp.
     *
     * Tạo socket, thiết lập timeout, và kết nối TCP tới máy chủ. Xử lý các lỗi liên quan.
     *
     * @param ip IP của máy chủ.
     * @param port Cổng của máy chủ.
     * @return true nếu kết nối thành công, false nếu thất bại.
     */
    bool connectToServer(const std::string &ip, uint16_t port)
    {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0)
        {
            perror("socket failed");
            return false;
        }

        struct timeval timeout;
        timeout.tv_sec = 5; // 1 giây
        timeout.tv_usec = 0;

        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        {
            perror("setsockopt failed");
        }

        sockaddr_in server_address;
        std::memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &server_address.sin_addr) <= 0)
        {
            perror("Invalid address/ Address not supported");
            return false;
        }

        if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        {
            perror("Connection Failed");
            return false;
        }

        std::cout << "Đã kết nối tới server trên: " << ip << ":" << port << std::endl;
        return true;
    }

    // Private constructor for Singleton
    NetworkClient() : socket_fd(-1)
    {
        if (!connectToServer(Const::SERVER_IP, Const::SERVER_PORT))
        {
            std::cerr << "Không thể kết nối tới server" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

public:
    // Delete copy constructor and assignment operator
    NetworkClient(const NetworkClient &) = delete;
    NetworkClient &operator=(const NetworkClient &) = delete;

    // Destructor
    ~NetworkClient()
    {
        closeConnection();
    }

    // Static method to get the singleton instance
    static NetworkClient &getInstance()
    {
        static NetworkClient instance;
        return instance;
    }

    /**
     * Gửi một gói tin tới server.
     *
     * @param messageType Kiểu của thông điệp.
     * @param payload Dữ liệu của gói tin.
     * @return Trả về true nếu gửi thành công, ngược lại trả về false.
     */
    bool sendPacket(MessageType messageType, const std::vector<uint8_t> &payload)
    {
        std::lock_guard<std::mutex> lock(send_mutex);

        Packet packet;
        packet.type = messageType;
        packet.length = htons(static_cast<uint16_t>(payload.size()));
        packet.payload = payload;

        std::vector<uint8_t> serialized = packet.serialize();

        ssize_t sent = send(socket_fd, serialized.data(), serialized.size(), 0);
        if (sent != static_cast<ssize_t>(serialized.size()))
        {
            perror("send failed");
            return false;
        }
        return true;
    }

    /**
     * Nhận một gói tin từ socket.
     *
     * @param packet Tham chiếu đến đối tượng Packet để lưu dữ liệu nhận được.
     * @return Trả về true nếu nhận thành công, false nếu không có dữ liệu hoặc kết nối đóng.
     */
    bool receivePacket(Packet &packet)
    {
        uint8_t temp_buffer[Const::BUFFER_SIZE];

        ssize_t bytes_received = recv(socket_fd, temp_buffer, sizeof(temp_buffer), 0);
        if (bytes_received < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                return false; // Không có dữ liệu để nhận trong thời gian chờ
            }
            perror("receive failed");
            return false;
        }
        else if (bytes_received == 0)
        {
            // Kết nối đã đóng, dừng chương trình
            std::cerr << "Kết nối tới server đã đóng." << std::endl;
            SessionData::getInstance().setRunning(false);
            return false;
        }

        buffer.insert(buffer.end(), temp_buffer, temp_buffer + bytes_received);

        while (buffer.size() >= 3)
        {
            MessageType type = static_cast<MessageType>(buffer[0]);
            uint16_t length = (static_cast<uint16_t>(buffer[1]) << 8) |
                              static_cast<uint16_t>(buffer[2]);

            length = ntohs(length); // Chuyển đổi sang host byte order

            if (buffer.size() < 3 + length)
            {
                break; // Chưa nhận đủ dữ liệu
            }

            std::vector<uint8_t> payload(buffer.begin() + 3, buffer.begin() + 3 + length);
            packet = Packet{type, length, payload};

            buffer.erase(buffer.begin(), buffer.begin() + 3 + length);
            return true;
        }

        return false; // Chưa nhận đủ dữ liệu
    }

    void closeConnection()
    {
        if (socket_fd != -1)
        {
            close(socket_fd);
            socket_fd = -1;
        }
    }
};

#endif // NETWORK_CLIENT_HPP
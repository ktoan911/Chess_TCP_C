#ifndef NETWORK_CLIENT_HPP
#define NETWORK_CLIENT_HPP

#include <string>
#include <vector>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#include "../common/protocol.hpp"
#include "../common/message.hpp"
#include "../common/utils.hpp"
#include "../common/const.hpp"

/**
 * @brief Singleton quản lý kết nối mạng TCP với máy chủ.
 *
 * Lớp NetworkClient là một singleton dùng để thiết lập và duy trì kết nối TCP với máy chủ.
 * Nó cung cấp các phương thức để gửi và nhận gói tin.
 */
class NetworkClient
{
private:
    int socket_fd;
    std::vector<uint8_t> buffer;

    /**
     * @brief Kết nối đến máy chủ với port discovery.
     *
     * Thử kết nối lần lượt các port từ base_port đến base_port + MAX_PORT_ATTEMPTS - 1
     * cho đến khi tìm được server đang chạy.
     *
     * @param ip IP của máy chủ.
     * @param base_port Cổng khởi đầu để thử kết nối.
     * @return true nếu kết nối thành công, false nếu thất bại.
     */
    bool connectToServer(const std::string &ip, uint16_t base_port)
    {
        uint16_t current_port = base_port;

        for (int attempt = 0; attempt < Const::MAX_PORT_ATTEMPTS; ++attempt)
        {
            // Tạo socket mới cho mỗi lần thử
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd < 0)
            {
                perror("socket failed");
                return false;
            }

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;  // 100ms - poll already handles waiting

            if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
            {
                perror("setsockopt failed");
            }

            sockaddr_in server_address;
            std::memset(&server_address, 0, sizeof(server_address));
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(current_port);

            if (inet_pton(AF_INET, ip.c_str(), &server_address.sin_addr) <= 0)
            {
                perror("Invalid address/ Address not supported");
                close(socket_fd);
                return false;
            }

            if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == 0)
            {
                // Kết nối thành công
                std::cout << "Đã kết nối tới server trên: " << ip << ":" << current_port << std::endl;
                return true;
            }

            // Kết nối thất bại, đóng socket và thử port tiếp theo
            close(socket_fd);
            socket_fd = -1;
            ++current_port;
        }

        // Không kết nối được với bất kỳ port nào
        std::cerr << "Không thể kết nối tới server trên các port "
                  << base_port << "-" << (base_port + Const::MAX_PORT_ATTEMPTS - 1) << std::endl;
        return false;
    }

    // Private constructor for Singleton
    NetworkClient() : socket_fd(-1)
    {
        if (!connectToServer(Const::SERVER_IP, Const::SERVER_PORT_BASE))
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
     * @brief Lấy socket file descriptor để sử dụng với select()
     * @return Socket fd hoặc -1 nếu chưa kết nối
     */
    int getSocketFd() const
    {
        return socket_fd;
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

        Packet packet;
        packet.type = messageType;
        packet.length = htons(static_cast<uint16_t>(payload.size()));
        packet.payload = payload;

        std::vector<uint8_t> serialized = packet.serialize();

        ssize_t sent = send(socket_fd, serialized.data(), serialized.size(), 0);
        if (sent != static_cast<ssize_t>(serialized.size()))
        {
            // make it more precise
            std::string err_msg = (sent < 0) ? std::strerror(errno) : "Incomplete send";
            std::cerr << "Gửi gói tin thất bại: " << err_msg << std::endl;
            
            return false;
        }
        return true;
    }

    /**
     * Nhận một gói tin từ socket.
     *
     * @param packet Tham chiếu đến đối tượng Packet để lưu dữ liệu nhận được.
     * @return 1 nếu nhận thành công, 0 nếu không có dữ liệu, -1 nếu kết nối đóng/lỗi
     */
    int receivePacket(Packet &packet)
    {
        // Kiểm tra buffer hiện tại TRƯỚC khi gọi recv()
        // Vì có thể recv() trước đó đã nhận nhiều packets cùng lúc
        if (buffer.size() >= Const::PACKET_HEADER_SIZE)
        {
            MessageType type = static_cast<MessageType>(buffer[0]);
            uint16_t length = (static_cast<uint16_t>(buffer[1]) << 8) |
                              static_cast<uint16_t>(buffer[2]);
            
            // Chuyển từ network byte order về host byte order
            length = ntohs(length);

            // Kiểm tra đủ dữ liệu cho payload
            if (buffer.size() >= Const::PACKET_HEADER_SIZE + length)
            {
                std::vector<uint8_t> payload(buffer.begin() + Const::PACKET_HEADER_SIZE, 
                                              buffer.begin() + Const::PACKET_HEADER_SIZE + length);
                packet = Packet{type, length, payload};
                buffer.erase(buffer.begin(), buffer.begin() + Const::PACKET_HEADER_SIZE + length);
                return 1; // Thành công - trả về packet từ buffer
            }
        }

        // de den duoc day => buffer 0 du du lieu de tao ra packet
        // Nhan them du lieu tu socket
        uint8_t temp_buffer[Const::BUFFER_SIZE];

        ssize_t bytes_received = recv(socket_fd, temp_buffer, sizeof(temp_buffer), 0);
        if (bytes_received < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                return 0; // Không có dữ liệu - poll lại sau
            }
            return -1; // Lỗi socket
        }
        else if (bytes_received == 0)
        {
            return -1; // Connection đóng
        }

        buffer.insert(buffer.end(), temp_buffer, temp_buffer + bytes_received);

        // Thử parse sau khi nhận thêm dữ liệu
        if (buffer.size() >= Const::PACKET_HEADER_SIZE)
        {
            MessageType type = static_cast<MessageType>(buffer[0]);
            uint16_t length = (static_cast<uint16_t>(buffer[1]) << 8) |
                              static_cast<uint16_t>(buffer[2]);
            
            // Chuyển từ network byte order về host byte order
            length = ntohs(length);

            // Kiểm tra đủ dữ liệu cho payload
            if (buffer.size() >= Const::PACKET_HEADER_SIZE + length)
            {
                std::vector<uint8_t> payload(buffer.begin() + Const::PACKET_HEADER_SIZE, 
                                              buffer.begin() + Const::PACKET_HEADER_SIZE + length);
                packet = Packet{type, length, payload};
                buffer.erase(buffer.begin(), buffer.begin() + Const::PACKET_HEADER_SIZE + length);
                return 1; // Thành công
            }
        }

        return 0; // still 0 du du lieu de tao packet
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
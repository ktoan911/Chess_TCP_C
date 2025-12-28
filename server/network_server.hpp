// ==================== HEADER GUARD ====================
// Đảm bảo file header này chỉ được include một lần trong quá trình biên dịch
// Tránh lỗi "multiple definition" khi file được include nhiều lần
#ifndef NETWORK_SERVER_HPP
#define NETWORK_SERVER_HPP

// ==================== THỨ VIỆN CHUẨN C++ ====================
#include <cstring>  // Để sử dụng các hàm xử lý chuỗi C như memset
#include <iostream> // Để in ra console (std::cout, std::cerr)
#include <mutex>    // Để đồng bộ hóa dữ liệu giữa các luồng (thread safety)
#include <unordered_map> // Để sử dụng bảng băm (hash map) lưu trữ thông tin clients
#include <vector> // Để sử dụng mảng động (dynamic array) cho buffer dữ liệu

// ==================== THỨ VIỆN SOCKET (LINUX) ====================
#include <arpa/inet.h> // Chứa các hàm chuyển đổi địa chỉ: inet_ntoa(), htons(), ntohs()
#include <netinet/in.h> // Định nghĩa cấu trúc sockaddr_in cho địa chỉ IPv4
#include <sys/socket.h> // Chứa các hàm socket cơ bản: socket(), bind(), listen(), accept(), send(), recv()
#include <unistd.h>     // Chứa hàm close() để đóng socket

// ==================== HEADER FILE TỰ ĐỊNH NGHĨA ====================
#include "../common/const.hpp" // Chứa các hằng số như SERVER_PORT, BUFFER_SIZE, BACKLOG
#include "../common/message.hpp" // Định nghĩa MessageType và các loại message
#include "../common/protocol.hpp" // Định nghĩa cấu trúc Packet và các hàm serialize/deserialize
#include "structs.hpp" // Định nghĩa các struct dùng chung

// ==================== NETWORKSERVER CLASS ====================
/**
 * @brief Lớp NetworkServer - Quản lý tất cả kết nối mạng của server.
 *
 * Đây là lớp Singleton (chỉ có duy nhất 1 instance trong toàn bộ chương trình).
 * Chức năng chính:
 * - Khởi tạo và quản lý server socket
 * - Chấp nhận kết nối từ clients
 * - Gửi/nhận dữ liệu với clients
 * - Quản lý thông tin của tất cả clients đang kết nối
 * - Đảm bảo thread-safe khi nhiều luồng truy cập đồng thời
 */
class NetworkServer {
private:
  // ==================== PRIVATE MEMBERS ====================

  /**
   * File descriptor của server socket
   * - Đây là số nguyên đại diện cho socket của server
   * - Được tạo bởi hàm socket() và dùng để lắng nghe kết nối từ clients
   * - Giá trị -1 nghĩa là socket chưa được khởi tạo hoặc đã bị đóng
   */
  int server_fd;

  /**
   * Bảng băm (hash map) lưu trữ thông tin của tất cả clients đang kết nối
   * - Key (khóa): file descriptor của client (int) - số định danh kết nối
   * - Value (giá trị): ClientInfo - thông tin chi tiết của client đó
   *
   * Ví dụ: clients[5] sẽ trả về thông tin của client có file descriptor = 5
   * Khi client ngắt kết nối, ta xóa khỏi map bằng clients.erase(fd)
   */
  std::unordered_map<int, ClientInfo> clients;

  /**
   * Mutex để bảo vệ map clients khỏi xung đột khi nhiều luồng truy cập
   * - Khi một luồng cần đọc/ghi clients map, nó phải lock mutex này
   * - Các luồng khác phải đợi cho đến khi mutex được unlock
   * - Đảm bảo tính toàn vẹn dữ liệu trong môi trường đa luồng (multi-threaded)
   */
  std::mutex clients_mutex;

  /**
   * @brief Khởi tạo máy chủ với cổng được chỉ định.
   *
   * Các bước thực hiện:
   * 1. Tạo socket mới với socket()
   * 2. Thiết lập địa chỉ server (IP + port)
   * 3. Gắn (bind) socket với địa chỉ đã thiết lập
   * 4. Bắt đầu lắng nghe (listen) các kết nối đến
   *
   * Nếu bất kỳ bước nào thất bại, chương trình sẽ in lỗi và thoát.
   *
   * @param port Số cổng mà server sẽ lắng nghe kết nối (thường là 8080, 3000,
   * v.v.)
   */
  void initialize(uint16_t port) {
    // ===== BƯỚC 1: TẠO SOCKET =====
    // socket(AF_INET, SOCK_STREAM, 0):
    // - AF_INET: Sử dụng IPv4
    // - SOCK_STREAM: Sử dụng TCP (connection-oriented, reliable)
    // - 0: Giao thức mặc định cho TCP
    // Trả về: file descriptor (số nguyên) nếu thành công, -1 nếu thất bại
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
      perror("socket failed"); // In ra thông báo lỗi
      exit(EXIT_FAILURE);      // Thoát chương trình với mã lỗi
    }

    // ===== BƯỚC 2: THIẾT LẬP ĐỊA CHỈ =====
    sockaddr_in address; // Cấu trúc chứa địa chỉ IP và port

    // Khởi tạo tất cả byte của address về 0
    // memset(&address, 0, sizeof(address)) đảm bảo không có dữ liệu rác
    std::memset(&address, 0, sizeof(address));

    // Thiết lập họ địa chỉ là IPv4
    address.sin_family = AF_INET;

    // INADDR_ANY = 0.0.0.0 - lắng nghe trên tất cả các network interface
    // (WiFi, Ethernet, localhost, v.v.)
    address.sin_addr.s_addr = INADDR_ANY;

    // htons() = Host TO Network Short
    // Chuyển đổi số port từ byte order của máy (host) sang network byte order
    // Network byte order là big-endian (byte cao trước)
    address.sin_port = htons(port);

    // ===== BƯỚC 3: BIND SOCKET VỚI ĐỊA CHỈ =====
    // Gắn socket với địa chỉ IP và port đã thiết lập
    // Sau khi bind thành công, socket sẽ "chiếm giữ" port này
    // Không chương trình nào khác có thể dùng port này cho đến khi ta đóng
    // socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      perror("bind failed"); // In lỗi (ví dụ: "Address already in use")
      close(server_fd);      // Đóng socket trước khi thoát
      exit(EXIT_FAILURE);
    }

    // ===== BƯỚC 4: BẮT ĐẦU LẮNG NGHE =====
    // listen(server_fd, backlog):
    // - server_fd: socket đã được bind
    // - Const::BACKLOG: Số lượng kết nối tối đa có thể chờ trong hàng đợi
    //   (nếu server đang bận xử lý, các kết nối mới sẽ chờ trong queue)
    if (listen(server_fd, Const::BACKLOG) < 0) {
      perror("listen failed");
      close(server_fd);
      exit(EXIT_FAILURE);
    }

    // In thông báo server đã sẵn sàng
    // inet_ntoa(): chuyển địa chỉ IP từ dạng số sang chuỗi (ví dụ:
    // "192.168.1.1") ntohs(): Network TO Host Short - chuyển port từ network
    // byte order về host byte order
    std::cout << "Server đang lắng nghe trên: " << inet_ntoa(address.sin_addr)
              << ":" << ntohs(address.sin_port) << " ..." << std::endl;
  }

  // ===== CONSTRUCTOR RIÊNG TƯ (PRIVATE) =====
  // Constructor được đặt private để ngăn tạo instance từ bên ngoài
  // Đây là đặc điểm của Singleton pattern - chỉ có thể tạo instance thông qua
  // getInstance()
  NetworkServer()
      : server_fd(-1) // Khởi tạo server_fd = -1 (chưa có socket)
  {
    // Gọi initialize với port mặc định từ file const.hpp
    initialize(Const::SERVER_PORT);
  }

public:
  // ==================== PUBLIC METHODS ====================

  // ===== XÓA COPY CONSTRUCTOR VÀ ASSIGNMENT OPERATOR =====
  // Ngăn việc sao chép (copy) đối tượng NetworkServer
  // Vì đây là Singleton, chỉ được phép có DUY NHẤT 1 instance
  // = delete có nghĩa là compiler sẽ báo lỗi nếu ai đó cố gắng copy
  NetworkServer(const NetworkServer &) = delete; // Ngăn copy constructor
  NetworkServer &
  operator=(const NetworkServer &) = delete; // Ngăn assignment operator

  // ===== DESTRUCTOR =====
  /**
   * @brief Destructor - Được gọi khi đối tượng NetworkServer bị hủy
   *
   * Đóng server socket nếu nó còn mở (-1 = đã đóng hoặc chưa mở)
   * Giải phóng tài nguyên hệ thống
   */
  ~NetworkServer() {
    if (server_fd != -1) // Nếu socket còn mở
    {
      close(server_fd); // Đóng socket
    }
  }

  // ===== PHƯƠNG THỨC LẤY INSTANCE (SINGLETON PATTERN) =====
  /**
   * @brief Lấy instance duy nhất của NetworkServer
   *
   * Đây là cách DUY NHẤT để truy cập đối tượng NetworkServer.
   * Lần đầu tiên gọi hàm này, đối tượng static sẽ được tạo.
   * Các lần sau, nó trả về cùng đối tượng đó (không tạo mới).
   *
   * Cách sử dụng:
   *   NetworkServer& server = NetworkServer::getInstance();
   *   server.acceptConnection();
   *
   * @return Tham chiếu đến instance duy nhất của NetworkServer
   */
  static NetworkServer &getInstance() {
    // static local variable - chỉ được khởi tạo MỘT LẦN duy nhất
    // Lần đầu gọi hàm: tạo instance mới
    // Các lần sau: trả về instance đã tạo trước đó
    static NetworkServer instance;
    return instance;
  }

  /**
   * @brief Chấp nhận kết nối mới từ client.
   *
   * Hàm này CHẶN (block) cho đến khi có client kết nối.
   * Khi có client kết nối, nó tạo một socket mới để giao tiếp với client đó.
   * Server socket (server_fd) vẫn tiếp tục lắng nghe các kết nối khác.
   *
   * Luồng hoạt động:
   * 1. Server gọi accept() và chờ (block)
   * 2. Client gọi connect() để kết nối đến server
   * 3. accept() trả về một client_fd mới để giao tiếp với client
   * 4. Server dùng client_fd này để send/recv dữ liệu với client
   *
   * @return File descriptor của client nếu thành công, -1 nếu thất bại.
   */
  int acceptConnection() {
    // Cấu trúc để lưu địa chỉ IP và port của client
    sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);

    // accept() sẽ:
    // - Chờ cho đến khi có client kết nối (blocking call)
    // - Tạo một socket mới (client_fd) để giao tiếp với client
    // - Lưu thông tin địa chỉ của client vào client_address
    // - Trả về client_fd (số dương) nếu thành công, -1 nếu lỗi
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_address, &client_len);
    if (client_fd < 0) {
      perror("accept failed"); // In lỗi nếu accept thất bại
      return -1;
    }

    // In thông tin client vừa kết nối (IP, port, file descriptor)
    std::cout << "Client kết nối từ: " << inet_ntoa(client_address.sin_addr)
              << ":" << ntohs(client_address.sin_port) << " (fd = " << client_fd
              << ")" << std::endl;
    return client_fd;
  }

  /**
   * @brief Gửi một gói tin đến client thông qua file descriptor.
   *
   * Các bước thực hiện:
   * 1. Tạo đối tượng Packet với messageType và payload
   * 2. Chuyển đổi (serialize) Packet thành dãy byte để gửi qua mạng
   * 3. Gửi dãy byte qua socket bằng hàm send()
   * 4. Kiểm tra xem đã gửi đủ byte chưa
   *
   * @param client_fd File descriptor của client nhận gói tin
   * @param messageType Loại thông điệp (LOGIN, REGISTER, MOVE, v.v.)
   * @param payload Dữ liệu thực tế của gói tin (đã được serialize từ Message)
   * @return true nếu gửi thành công toàn bộ dữ liệu, false nếu thất bại
   */
  bool sendPacket(int client_fd, MessageType messageType,
                  const std::vector<uint8_t> &payload) {
    // Tạo đối tượng Packet
    Packet packet;
    packet.type = messageType; // Loại message (1 byte)

    // Chuyển độ dài payload sang network byte order (big-endian)
    // htons = Host TO Network Short (chuyển uint16_t sang network order)
    // playload.size là số byte của payload số thường *
    packet.length = htons(static_cast<uint16_t>(payload.size()));

    packet.payload = payload; // Dữ liệu thực tế

    // Serialize packet thành dãy byte:
    // [1 byte type][2 byte length][N byte payload]
    std::vector<uint8_t> serialized = packet.serialize();

    // Gửi dữ liệu qua socket
    // send(socket, buffer, size, flags)
    // - Trả về số byte đã gửi thành công
    // - Có thể gửi ít hơn số byte yêu cầu nếu buffer đầy
    // .data trả về con trỏ đến byte đầu tiên vector
    ssize_t sent = send(client_fd, serialized.data(), serialized.size(), 0);

    // Kiểm tra xem đã gửi đủ không
    if (sent != static_cast<ssize_t>(serialized.size())) {
      perror("send failed"); // In lỗi nếu send thất bại hoặc gửi thiếu
      return false;
    }
    return true; // Gửi thành công
  }

  /**
   * @brief Gửi gói tin đến client thông qua tên đăng nhập (username).
   *
   * Hàm này tìm kiếm client theo username, sau đó gọi sendPacket() với file
   * descriptor. Tiện lợi hơn khi ta chỉ biết username mà không biết file
   * descriptor.
   *
   * Ví dụ: Gửi thông báo thách đấu đến người chơi "player123"
   *
   * @param username Tên đăng nhập của người dùng cần gửi gói tin
   * @param messageType Loại thông điệp cần gửi
   * @param payload Dữ liệu gói tin cần gửi
   * @return true nếu tìm thấy user và gửi thành công, false nếu không tìm thấy
   * hoặc gửi thất bại
   */
  bool sendPacketToUsername(const std::string &username,
                            MessageType messageType,
                            const std::vector<uint8_t> &payload) {
    // Khóa clients_mutex để tránh xung đột khi đọc clients map
    std::lock_guard<std::mutex> lock(clients_mutex);

    // Duyệt qua tất cả clients để tìm username khớp
    // pair.first = client_fd (int)
    // pair.second = ClientInfo (có thuộc tính username)
    for (const auto &pair : clients) {
      if (pair.second.username == username) // Tìm thấy username
      {
        // Gửi packet bằng file descriptor
        return sendPacket(pair.first, messageType, payload);
      }
    }

    // Không tìm thấy username trong danh sách clients
    std::cerr << "Username " << username << " không được tìm thấy."
              << std::endl;
    return false;
  }

  /**
   * @brief Nhận một gói tin từ client.
   *
   * ĐÂY LÀ HÀM PHỨC TẠP NHẤT TRONG CLASS!
   *
   * Vấn đề cần giải quyết:
   * - TCP là stream protocol (dòng dữ liệu liên tục), không có ranh giới gói
   * tin
   * - Một lần recv() có thể nhận được:
   *   + Chưa đủ một packet (cần chờ thêm)
   *   + Đúng một packet
   *   + Nhiều packet cùng lúc
   *   + Một phần của packet (phần còn lại đến sau)
   *
   * Giải pháp: Sử dụng buffer để lưu trữ dữ liệu
   * - Mỗi client có buffer riêng (ClientInfo.buffer)
   * - Dữ liệu nhận được được thêm vào buffer
   * - Khi đủ dữ liệu để tạo thành một packet hoàn chỉnh, ta lấy ra và xử lý
   * - Phần dữ liệu thừa (nếu có) vẫn ở lại buffer cho lần xử lý sau
   *
   * Cấu trúc packet:
   * [1 byte: MessageType][2 bytes: Length][Length bytes: Payload]
   *
   * @param client_fd File descriptor của client gửi dữ liệu
   * @param packet Tham chiếu đến đối tượng Packet để lưu gói tin nhận được
   * @return true nếu nhận được một packet hoàn chỉnh, false nếu chưa đủ dữ liệu
   * hoặc kết nối đóng
   */
  bool receivePacket(int client_fd, Packet &packet) {
    // ===== BƯỚC 1: NHẬN DỮ LIỆU TỪ SOCKET =====
    // Đọc dữ liệu từ socket mà KHÔNG GIỮ KHÓA (để tránh block các luồng khác)
    // Buffer tạm để nhận dữ liệu từ socket
    uint8_t buffer_temp[Const::BUFFER_SIZE];

    // recv() nhận dữ liệu từ socket:
    // - Trả về số byte đã nhận (> 0) nếu thành công
    // - Trả về 0 nếu client đóng kết nối
    // - Trả về -1 nếu có lỗi
    ssize_t bytes_received =
        recv(client_fd, buffer_temp, sizeof(buffer_temp), 0);
    if (bytes_received <= 0) {
      // bytes_received == 0: Client đã đóng kết nối (graceful shutdown)
      // bytes_received < 0: Có lỗi xảy ra (connection reset, timeout, v.v.)
      return false;
    }

    // ===== BƯỚC 2: THÊM DỮ LIỆU VÀO BUFFER CỦA CLIENT =====
    // Bảo vệ clients map khi thêm dữ liệu vào buffer
    {
      // Lock mutex để đảm bảo không có luồng khác đang truy cập clients map
      std::lock_guard<std::mutex> lock(clients_mutex);

      // Thêm dữ liệu vừa nhận vào cuối buffer của client
      // insert(vị_trí, từ_đâu, đến_đâu)
      // buffer_temp + bytes_received là con trỏ đến cuối dữ liệu nhận được
      clients[client_fd].buffer.insert(
          clients[client_fd].buffer.end(), // Thêm vào cuối buffer
          buffer_temp,                     // Từ đầu buffer_temp
          buffer_temp + bytes_received     // Đến cuối dữ liệu nhận được
      );
    } // Mutex tự động unlock khi ra khỏi scope

    // ===== BƯỚC 3: XỬ LÝ BUFFER ĐỂ TẠO PACKET =====
    {
      // Bảo vệ buffer của client_fd cụ thể (không phải toàn bộ clients map)
      // Điều này cho phép các luồng khác xử lý clients khác đồng thời
      std::lock_guard<std::mutex> lock(clients[client_fd].mutex);

      // Lấy tham chiếu đến buffer để code ngắn gọn hơn
      auto &buffer = clients[client_fd].buffer;

      // Vòng lặp xử lý buffer (có thể có nhiều packet trong buffer)
      while (buffer.size() >= 3) // Cần ít nhất 3 byte (1 type + 2 length)
      {
        // ===== BƯỚC 3.1: ĐỌC HEADER (TYPE VÀ LENGTH) =====

        // Đọc MessageType (byte đầu tiên)
        MessageType type = static_cast<MessageType>(buffer[0]);

        // Đọc Length (2 byte tiếp theo)
        // buffer[1] là byte cao (high byte), buffer[2] là byte thấp (low byte)
        // Ghép 2 byte lại:
        //   - buffer[1] << 8: dịch byte cao sang trái 8 bit
        //   - buffer[2]: byte thấp
        //   - Ghép lại bằng toán tử OR (|)
        uint16_t length = (static_cast<uint16_t>(buffer[1]) << 8) |
                          static_cast<uint16_t>(buffer[2]);

        // Chuyển từ network byte order (big-endian) về host byte order
        length = ntohs(length);

        // ===== BƯỚC 3.2: KIỂM TRA ĐỦ DỮ LIỆU CHƯA =====

        // Kiểm tra xem buffer có đủ dữ liệu cho toàn bộ packet không
        // Cần: 3 byte (header) + length byte (payload)
        if (buffer.size() < 3 + length) {
          break; // Chưa đủ dữ liệu, thoát vòng lặp và chờ recv() lần sau
        }

        // ===== BƯỚC 3.3: TRÍCH XUẤT PAYLOAD =====

        // Tạo vector chứa payload bằng cách copy dữ liệu từ buffer
        // Từ vị trí buffer.begin() + 3 (sau header)
        // Đến vị trí buffer.begin() + 3 + length (hết payload)
        std::vector<uint8_t> payload(buffer.begin() + 3,
                                     buffer.begin() + 3 + length);

        // Gán dữ liệu vào packet trả về
        packet = Packet{type, length, payload};

        // ===== BƯỚC 3.4: XÓA DỮ LIỆU ĐÃ XỬ LÝ KHỎI BUFFER =====

        // Xóa phần đã xử lý (3 byte header + length byte payload)
        // Phần dữ liệu còn lại (nếu có) sẽ được xử lý ở lần lặp tiếp theo
        buffer.erase(buffer.begin(), buffer.begin() + 3 + length);

        return true; // Đã nhận được một packet hoàn chỉnh
      }
    } // Mutex tự động unlock

    // Nếu đến đây nghĩa là:
    // - Đã nhận dữ liệu và thêm vào buffer
    // - Nhưng chưa đủ để tạo thành một packet hoàn chỉnh
    // - Cần gọi receivePacket() lần nữa khi có thêm dữ liệu
    return false; // Chưa nhận đủ dữ liệu cho một packet hoàn chỉnh
  }

  // ===== PHƯƠNG THỨC QUẢN LÝ USERNAME =====

  /**
   * @brief Gán username cho client.
   *
   * Sau khi client đăng nhập thành công, gọi hàm này để lưu username.
   * Dùng để nhận diện client theo tên thay vì file descriptor.
   *
   * @param client_fd File descriptor của client
   * @param username Tên đăng nhập của client
   */
  void setUsername(int client_fd, const std::string &username) {
    // Khóa clients_mutex để bảo vệ khi ghi dữ liệu
    std::lock_guard<std::mutex> lock(clients_mutex);
    // Gán username cho client tương ứng với client_fd
    clients[client_fd].username = username;
  }

  /**
   * @brief Lấy username của client theo file descriptor.
   *
   * @param client_fd File descriptor của client
   * @return Tên đăng nhập của client, hoặc chuỗi rỗng nếu chưa đăng nhập
   */
  std::string getUsername(int client_fd) {
    // Khóa clients_mutex để bảo vệ khi đọc dữ liệu
    std::lock_guard<std::mutex> lock(clients_mutex);
    return clients[client_fd].username;
  }

  /**
   * @brief Tìm file descriptor của client theo username.
   *
   * Hàm ngược với getUsername() - tìm fd từ username.
   * Hữu ích khi cần gửi dữ liệu đến client nhưng chỉ biết username.
   *
   * @param username Tên đăng nhập cần tìm
   * @return File descriptor của client nếu tìm thấy, -1 nếu không tìm thấy
   */
  int getClientFD(const std::string &username) {
    // Khóa clients_mutex để bảo vệ khi duyệt qua map
    std::lock_guard<std::mutex> lock(clients_mutex);

    // Duyệt qua tất cả clients để tìm username khớp
    for (const auto &pair : clients) {
      if (pair.second.username == username) {
        return pair.first; // Trả về file descriptor
      }
    }
    return -1; // Không tìm thấy
  }

  // ===== PHƯƠNG THỨC LẤY ĐỊA CHỈ IP =====

  /**
   * @brief Lấy địa chỉ IP của client theo file descriptor.
   *
   * Sử dụng getpeername() để lấy thông tin địa chỉ của peer (client) đang kết
   * nối. Hữu ích cho việc ghi log, kiểm tra bảo mật, v.v.
   *
   * @param client_fd File descriptor của client.
   * @return Địa chỉ IP dưới dạng chuỗi (ví dụ: "192.168.1.100"), hoặc rỗng nếu
   * thất bại.
   */
  std::string getClientIP(int client_fd) {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // getpeername() lấy thông tin địa chỉ của peer (client) đang kết nối
    // Trả về 0 nếu thành công, -1 nếu thất bại
    if (getpeername(client_fd, (struct sockaddr *)&addr, &addr_len) == 0) {
      // inet_ntoa() chuyển địa chỉ IP từ dạng số sang dạng chuỗi
      return std::string(inet_ntoa(addr.sin_addr));
    }
    return ""; // Thất bại, trả về chuỗi rỗng
  }

  /**
   * @brief Lấy địa chỉ IP của client theo username.
   *
   * Kết hợp getClientFD() và getClientIP() để lấy IP từ username.
   * Tiện lợi khi chỉ biết username của người chơi.
   *
   * @param username Tên người dùng.
   * @return Địa chỉ IP dưới dạng chuỗi, hoặc rỗng nếu không tìm thấy.
   */
  std::string getClientIPByUsername(const std::string &username) {
    // Tìm file descriptor từ username
    int client_fd = getClientFD(username);
    if (client_fd != -1) // Nếu tìm thấy
    {
      // Lấy IP từ file descriptor
      return getClientIP(client_fd);
    }
    return ""; // Không tìm thấy username
  }

  // ===== PHƯƠNG THỨC KIỂM TRA TRẠNG THÁI =====

  /**
   * @brief Kiểm tra xem user có đang đăng nhập không.
   *
   * Duyệt qua tất cả clients để tìm xem có ai có username trùng khớp không.
   * Hữu ích để ngăn người dùng đăng nhập nhiều lần cùng lúc.
   *
   * @param username Tên đăng nhập cần kiểm tra
   * @return true nếu user đang online, false nếu không
   */
  bool isUserLoggedIn(const std::string &username) {
    // Khóa clients_mutex để bảo vệ khi duyệt qua map
    std::lock_guard<std::mutex> lock(clients_mutex);

    // Duyệt qua tất cả clients
    for (const auto &pair : clients) {
      if (pair.second.username == username) {
        return true; // Tìm thấy, user đang online
      }
    }
    return false; // Không tìm thấy, user không online
  }

  /**
   * @brief Kiểm tra xem client có còn kết nối không.
   *
   * Kiểm tra xem client_fd có tồn tại trong clients map không.
   * Nếu có trong map = đang kết nối, không có = đã ngắt kết nối.
   *
   * @param client_fd File descriptor của client.
   * @return true nếu client còn kết nối, false nếu không.
   */
  bool isClientConnected(int client_fd) {
    // Khóa clients_mutex để bảo vệ khi truy cập map
    std::lock_guard<std::mutex> lock(clients_mutex);

    // find() tìm kiếm client_fd trong map
    // Nếu tìm thấy: trả về iterator khác clients.end()
    // Nếu không tìm thấy: trả về clients.end()
    return clients.find(client_fd) != clients.end();
  }

  // ===== PHƯƠNG THỨC ĐÓNG KẾT NỐI =====

  /**
   * @brief Đóng kết nối với một client cụ thể.
   *
   * Các bước thực hiện:
   * 1. Đóng socket của client bằng close()
   * 2. Xóa thông tin client khỏi clients map
   *
   * Nên gọi hàm này khi:
   * - Client ngắt kết nối (recv() trả về 0)
   * - Client gửi dữ liệu không hợp lệ
   * - Server muốn kick client ra
   *
   * @param client_fd File descriptor của client cần đóng
   */
  void closeConnection(int client_fd) {
    // Đóng socket - giải phóng file descriptor
    close(client_fd);

    // Xóa thông tin client khỏi clients map
    {
      // Khóa clients_mutex để bảo vệ khi xóa khỏi map
      std::lock_guard<std::mutex> lock(clients_mutex);
      // erase() xóa entry với key = client_fd
      clients.erase(client_fd);
    }
  }

  /**
   * @brief Đóng tất cả kết nối và dừng server.
   *
   * Đóng:
   * - Server socket (không chấp nhận kết nối mới nữa)
   * - Tất cả client sockets
   * - Xóa toàn bộ clients map
   *
   * Thường được gọi khi tắt server (cleanup).
   */
  void closeAllConnections() {
    // Khóa clients_mutex để bảo vệ toàn bộ quá trình
    std::lock_guard<std::mutex> lock(clients_mutex);

    // Đóng server socket - không nhận kết nối mới nữa
    close(server_fd);

    // Duyệt qua tất cả clients và đóng từng socket
    for (auto &pair : clients) {
      close(pair.first); // pair.first là client_fd
    }

    // Xóa toàn bộ clients map (giải phóng bộ nhớ)
    clients.clear();
  }

  /**
   * @brief Giải phóng toàn bộ tài nguyên của server.
   *
   * CHÚ Ý: Hàm này có vấn đề!
   * - delete &getInstance() là UNDEFINED BEHAVIOR vì getInstance() trả về
   * static local variable
   * - Không nên delete static object
   * - Chỉ gọi closeAllConnections() là đủ
   *
   * Khuyến nghị: Chỉ gọi closeAllConnections() thay vì dispose()
   */
  void dispose() {
    closeAllConnections();
    // CHÚ Ý: Dòng dưới đây LÀ LỐI - không nên delete static object!
    delete &getInstance();
  }
}; // Kết thúc class NetworkServer

// Kết thúc header guard - đóng #ifndef NETWORK_SERVER_HPP ở đầu file
#endif // NETWORK_SERVER_HPP
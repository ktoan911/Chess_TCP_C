# 50 Câu Hỏi Phỏng Vấn Về Server Chess TCP (Chi Tiết & Chuyên Sâu)

Tài liệu này cung cấp 50 câu hỏi phỏng vấn xoay quanh source code server, đi kèm với câu trả lời chi tiết, giải thích cặn kẽ bản chất vấn đề (Deep Dive) để bạn nắm vững kiến thức Lập trình mạng và C++.

---

## I. Kiến trúc & Thiết kế (Architecture & Design)

### 1. Mô hình server hiện tại đang sử dụng là gì? (Thread-per-client, Event-driven, hay I/O Multiplexing?)
*   **Trả lời:** Server sử dụng mô hình **Thread-per-client** (Mỗi client một luồng).
*   **Bản chất chi tiết:**
    *   Trong `server_main.cpp`, vòng lặp `while(true)` liên tục gọi `accept()`. Khi có một client mới kết nối thành công, server ngay lập tức tạo một `std::thread` mới (`client_threads.emplace_back`) để chạy hàm `handleClient` riêng cho client đó.
    *   **Cơ chế:** Hệ điều hành sẽ cấp phát một Stack riêng và một Context (ngữ cảnh) cho luồng mới này. Luồng này sẽ chạy độc lập, thực hiện vòng lặp `recv` -> `process` -> `send` cho đến khi client ngắt kết nối.
    *   **Tại sao dùng ở đây:** Dễ cài đặt, code logic tuần tự, dễ hiểu, phù hợp với bài tập hoặc ứng dụng có số lượng user vừa phải (< 1000).

### 2. Tại sao `NetworkServer` lại được thiết kế theo mẫu Singleton?
*   **Trả lời:** Để đảm bảo tính duy nhất và truy cập toàn cục.
*   **Bản chất chi tiết:**
    *   **Tài nguyên độc quyền:** Server chỉ có thể lắng nghe trên một cổng (port) duy nhất tại một thời điểm. Nếu tạo 2 instance `NetworkServer` cùng lắng nghe port 8080, instance thứ 2 sẽ bị lỗi `bind failed: Address already in use`.
    *   **Quản lý tập trung:** Cần một nơi duy nhất quản lý danh sách `clients` (các kết nối đang active). Nếu có nhiều instance, dữ liệu về user online sẽ bị phân tán, không thể kiểm tra trùng lặp user hay ghép cặp đấu (matchmaking) chính xác.
    *   **Global Access:** Các thành phần khác như `GameManager` hay `MessageHandler` cần gọi `NetworkServer` để gửi gói tin ở bất kỳ đâu, Singleton giúp truy cập dễ dàng qua `NetworkServer::getInstance()`.

### 3. Làm thế nào để các thành phần `NetworkServer`, `DataStorage`, `GameManager` giao tiếp với nhau?
*   **Trả lời:** Thông qua kỹ thuật **Dependency Injection (DI)**.
*   **Bản chất chi tiết:**
    *   Thay vì `MessageHandler` tự tạo mới (new) các đối tượng `NetworkServer` hay `DataStorage` bên trong nó, chúng được truyền vào thông qua **Constructor** (như trong `message_handler.hpp`).
    *   **Lợi ích:** Giảm sự phụ thuộc chặt chẽ (Decoupling). `MessageHandler` không cần biết cách khởi tạo `NetworkServer` như thế nào, nó chỉ cần biết cách sử dụng. Điều này cực kỳ quan trọng cho việc Unit Test (có thể truyền vào các Mock Object giả lập server để test logic xử lý tin nhắn mà không cần mở kết nối mạng thật).

### 4. Ưu điểm và nhược điểm của mô hình Thread-per-client mà server đang dùng là gì?
*   **Trả lời:** Ưu điểm là đơn giản; Nhược điểm là khó mở rộng (Scalability kém).
*   **Bản chất chi tiết:**
    *   **Ưu điểm:** Lập trình tư duy theo lối tuần tự (Synchronous). Bạn viết code đọc dữ liệu, xử lý, rồi gửi lại ngay trong một hàm, không cần chia nhỏ logic thành các callback hay state machine phức tạp như Event-driven.
    *   **Nhược điểm (Vấn đề C10K):** Mỗi thread tiêu tốn bộ nhớ cho Stack (thường là 1MB - 8MB tùy OS) và tài nguyên kernel. Khi số lượng client tăng lên hàng ngàn (10.000), server sẽ hết RAM hoặc CPU sẽ tốn quá nhiều thời gian để chuyển đổi ngữ cảnh (Context Switching) giữa các luồng thay vì xử lý công việc thực sự.

### 5. Tại sao hàm `dispose()` trong `NetworkServer` lại bị đánh dấu là có vấn đề (bad practice)?
*   **Trả lời:** Vì việc `delete` một biến static local dẫn đến **Undefined Behavior** (Hành vi không xác định).
*   **Bản chất chi tiết:**
    *   Trong C++, biến static local (trong `getInstance`) được quản lý vòng đời bởi chương trình. Nó tự động hủy khi chương trình kết thúc (`main` return).
    *   Việc gọi `delete &instance` là cố gắng giải phóng vùng nhớ mà bạn không cấp phát thủ công bằng `new` (hoặc vùng nhớ đó nằm trong vùng Data Segment/BSS chứ không phải Heap). Điều này có thể gây crash chương trình hoặc lỗi bộ nhớ nghiêm trọng (Double Free).

### 6. Vai trò của lớp `MessageHandler` là gì? Tại sao không xử lý message ngay trong `NetworkServer`?
*   **Trả lời:** Để tuân thủ nguyên lý **Single Responsibility Principle (SRP)** - Nguyên lý đơn nhiệm.
*   **Bản chất chi tiết:**
    *   `NetworkServer` chỉ nên chịu trách nhiệm về các vấn đề "Mạng" (Networking): mở socket, chấp nhận kết nối, nhận byte thô, gửi byte thô. Nó không nên biết về "Luật chơi cờ" hay "Đăng nhập".
    *   `MessageHandler` chịu trách nhiệm về "Logic nghiệp vụ" (Business Logic): Phân tích gói tin là loại gì (Login hay Move), kiểm tra mật khẩu, cập nhật bàn cờ.
    *   Việc tách biệt giúp code dễ bảo trì: Nếu muốn đổi luật chơi, chỉ sửa `MessageHandler`. Nếu muốn đổi thư viện mạng, chỉ sửa `NetworkServer`.

### 7. Server quản lý danh sách các client đang kết nối bằng cấu trúc dữ liệu nào? Tại sao?
*   **Trả lời:** Sử dụng `std::unordered_map<int, ClientInfo>`.
*   **Bản chất chi tiết:**
    *   **Key (int):** Là File Descriptor (Socket ID). Đây là định danh duy nhất cho mỗi kết nối ở cấp độ hệ điều hành.
    *   **Value (ClientInfo):** Chứa thông tin user (username, buffer, mutex...).
    *   **Tại sao dùng Hash Map (`unordered_map`):** Thao tác tìm kiếm, chèn, xóa client diễn ra liên tục. Hash Map cho tốc độ trung bình là **O(1)**. Nếu dùng `std::vector` hay `std::list`, việc tìm client để gửi tin nhắn sẽ tốn O(N), làm chậm server khi đông người.

### 8. Cơ chế "Graceful Shutdown" của server được thực hiện như thế nào?
*   **Trả lời:** Đóng socket server trước, sau đó đóng lần lượt các socket client.
*   **Bản chất chi tiết:**
    *   Khi server muốn tắt, nó gọi `close(server_fd)` để ngừng nhận kết nối mới.
    *   Sau đó, nó duyệt qua map `clients`, gọi `close(client_fd)` cho từng client. Điều này gửi gói tin **FIN** (trong giao thức TCP) đến client, báo hiệu "Tôi sắp ngắt kết nối, bạn hãy dọn dẹp đi".
    *   Nếu tắt đột ngột (kill process), hệ điều hành sẽ đóng dùng, nhưng client có thể nhận lỗi "Connection Reset" thay vì đóng êm đẹp.

---

## II. Lập trình Socket (Socket Programming)

### 9. Hàm `socket(AF_INET, SOCK_STREAM, 0)` có ý nghĩa gì?
*   **Trả lời:** Tạo một endpoint giao tiếp mạng.
*   **Bản chất chi tiết:**
    *   `AF_INET`: Address Family Internet - Chỉ định sử dụng giao thức **IPv4** (32-bit IP address).
    *   `SOCK_STREAM`: Socket Type Stream - Chỉ định sử dụng giao thức truyền tải tin cậy, hướng kết nối, đảm bảo thứ tự -> Chính là **TCP**. (Nếu dùng UDP sẽ là `SOCK_DGRAM`).
    *   `0`: Protocol - Để mặc định, hệ thống tự chọn giao thức phù hợp với 2 tham số trước (TCP cho SOCK_STREAM).
    *   Hàm này trả về một `int` (File Descriptor) - một chỉ số trong bảng quản lý file của kernel, đại diện cho socket này.

### 10. Tại sao phải dùng `htons()` khi gán port cho `sockaddr_in`?
*   **Trả lời:** Để chuyển đổi Byte Order (Thứ tự byte).
*   **Bản chất chi tiết:**
    *   **Host Byte Order:** Máy tính (Intel/AMD x86) thường lưu số đa byte theo chuẩn **Little Endian** (byte thấp đứng trước). Ví dụ số `0x1234` lưu là `34 12`.
    *   **Network Byte Order:** Chuẩn mạng Internet quy định dùng **Big Endian** (byte cao đứng trước). Số `0x1234` phải gửi là `12 34`.
    *   `htons` (Host TO Network Short) đảo ngược thứ tự byte nếu cần thiết. Nếu không dùng, port 8080 (0x1F90) có thể bị hiểu nhầm thành port 36895 (0x901F) ở phía bên kia.

### 11. Hàm `bind()` có tác dụng gì? Chuyện gì xảy ra nếu bind vào port đã có tiến trình khác sử dụng?
*   **Trả lời:** Đăng ký quyền sở hữu địa chỉ IP và Port cho socket.
*   **Bản chất chi tiết:**
    *   Socket khi mới tạo chỉ là một tài nguyên vô danh. `bind()` gán cho nó một "địa chỉ nhà" cụ thể (VD: IP 0.0.0.0, Port 5555).
    *   Hệ điều hành quản lý bảng các port đang mở. Nếu port 5555 đã có tiến trình khác `bind` rồi, OS sẽ từ chối và trả về lỗi `EADDRINUSE` (Address already in use). Hai tiến trình không thể cùng lắng nghe một port TCP trên cùng một IP (trừ khi dùng `SO_REUSEPORT`).

### 12. Tham số `backlog` trong hàm `listen()` có ý nghĩa gì?
*   **Trả lời:** Kích thước hàng đợi các kết nối đang chờ (Pending Connections Queue).
*   **Bản chất chi tiết:**
    *   Khi client gọi `connect()`, quá trình bắt tay 3 bước (3-way handshake) diễn ra.
    *   Nếu server chưa kịp gọi `accept()`, các kết nối đã hoàn tất bắt tay sẽ được OS lưu vào một hàng đợi.
    *   `backlog` quy định số lượng tối đa kết nối nằm trong hàng đợi này. Nếu hàng đợi đầy, các client tiếp theo gọi `connect()` sẽ bị từ chối (nhận lỗi `ECONNREFUSED` hoặc timeout).

### 13. Hàm `accept()` hoạt động theo cơ chế Blocking hay Non-blocking trong code này?
*   **Trả lời:** **Blocking** (Chặn).
*   **Bản chất chi tiết:**
    *   Khi luồng chính gọi `accept()`, nếu chưa có client nào kết nối, luồng đó sẽ bị **ngủ (sleep)**. Nó không tiêu tốn CPU, nhưng cũng không thể làm việc khác.
    *   Khi có kết nối mới vào hàng đợi, OS sẽ đánh thức luồng này dậy, `accept()` trả về `client_fd` mới và luồng tiếp tục chạy.

### 14. Sự khác biệt giữa `server_fd` và `client_fd` trả về từ `accept()` là gì?
*   **Trả lời:** Một cái để nghe, một cái để nói chuyện.
*   **Bản chất chi tiết:**
    *   `server_fd` (Listening Socket): Chỉ dùng để thiết lập cấu hình và chờ đợi kết nối mới. Nó tồn tại suốt đời server. Không bao giờ dùng nó để gửi/nhận dữ liệu.
    *   `client_fd` (Connected Socket): Được sinh ra *mỗi khi* có một client kết nối thành công. Nó đại diện cho đường ống dữ liệu riêng biệt giữa server và client đó. Có 100 client thì có 100 `client_fd` khác nhau.

### 15. Hàm `recv()` trả về 0 có ý nghĩa gì?
*   **Trả lời:** Kết nối đã bị đóng bởi phía bên kia (End of File / Graceful Shutdown).
*   **Bản chất chi tiết:**
    *   Trong TCP, khi client gọi `close()`, nó gửi gói tin **FIN**.
    *   Server nhận được FIN, TCP stack của server hiểu là "bên kia không gửi gì nữa đâu".
    *   Khi ứng dụng gọi `recv()`, thay vì block chờ dữ liệu, nó trả về ngay giá trị 0. Đây là tín hiệu chuẩn để server biết cần dọn dẹp và đóng socket phía mình.

### 16. Tại sao cần dùng `INADDR_ANY` khi cấu hình địa chỉ server?
*   **Trả lời:** Để lắng nghe trên mọi Card mạng (Interfaces).
*   **Bản chất chi tiết:**
    *   Một máy chủ có thể có nhiều IP: Localhost (127.0.0.1), IP LAN (192.168.1.5), IP Public (14.232...).
    *   Nếu bind vào `127.0.0.1`, chỉ máy đó mới kết nối được.
    *   `INADDR_ANY` (giá trị 0) bảo OS: "Hãy chấp nhận kết nối đến port này bất kể nó đi vào từ IP nào của máy tôi".

### 17. Làm thế nào để lấy địa chỉ IP của client từ `client_fd`?
*   **Trả lời:** Dùng hàm `getpeername()`.
*   **Bản chất chi tiết:**
    *   Kernel lưu trữ thông tin về 2 đầu của kết nối TCP (Local Address và Remote Address).
    *   `getpeername(fd, ...)` truy vấn kernel để lấy struct `sockaddr_in` chứa IP và Port của đầu bên kia (Remote Peer). Sau đó dùng `inet_ntoa` để đổi số IP sang chuỗi (VD: "192.168.1.10").

### 18. Tại sao lại cần hàm `setsockopt` với `SO_REUSEADDR`?
*   **Trả lời:** Để server có thể khởi động lại ngay lập tức sau khi tắt.
*   **Bản chất chi tiết:**
    *   Khi server tắt, socket chuyển sang trạng thái **TIME_WAIT** trong khoảng 1-2 phút để đảm bảo mọi gói tin lạc trôi trên mạng đều đã đến nơi.
    *   Trong thời gian này, port vẫn bị coi là "đang bận". Nếu bật lại server ngay, `bind()` sẽ lỗi.
    *   `SO_REUSEADDR` cho phép chiếm dụng lại port ngay cả khi nó đang ở trạng thái TIME_WAIT.

---

## III. Giao thức TCP & Xử lý dữ liệu (Protocol & Data Handling)

### 19. Tại sao TCP được gọi là "Stream Protocol"? Điều này ảnh hưởng gì đến việc nhận dữ liệu?
*   **Trả lời:** Dữ liệu là dòng chảy liên tục, không phân chia ranh giới.
*   **Bản chất chi tiết:**
    *   TCP đảm bảo thứ tự byte: Gửi A, B, C thì nhận A, B, C.
    *   Nhưng TCP không bảo toàn ranh giới `send`: Bạn gọi `send("Hello")` rồi `send("World")`. Bên nhận có thể nhận được 1 cục "HelloWorld", hoặc "Hel", rồi "loWor", rồi "ld".
    *   **Ảnh hưởng:** Server không thể giả định 1 lần gọi `recv` sẽ nhận được đúng 1 gói tin logic. Phải xử lý việc ghép nối và cắt dữ liệu thủ công.

### 20. Server giải quyết vấn đề "TCP Stickiness" (dính gói) và phân mảnh gói tin như thế nào?
*   **Trả lời:** Dùng cơ chế **Buffering** (Đệm) và **Message Header** (Độ dài).
*   **Bản chất chi tiết:**
    *   Mỗi client có một `std::vector<uint8_t> buffer`.
    *   Mọi dữ liệu `recv` được đều `insert` vào cuối buffer này.
    *   Vòng lặp xử lý sẽ kiểm tra đầu buffer: Đọc `Length` (2 byte).
    *   Nếu `buffer.size() >= Length + HeaderSize`, nghĩa là đã đủ 1 gói. Cắt gói đó ra xử lý, xóa khỏi buffer. Lặp lại cho đến khi buffer không đủ dữ liệu cho gói tiếp theo.

### 21. Cấu trúc của một `Packet` trong code này gồm những phần nào?
*   **Trả lời:** Header (Type + Length) và Payload.
*   **Bản chất chi tiết:**
    *   **Type (1 byte):** Định danh loại gói tin (Login, Move, Error...).
    *   **Length (2 bytes):** Cho biết phần dữ liệu phía sau dài bao nhiêu byte. Đây là mấu chốt để giải quyết vấn đề Stream Protocol.
    *   **Payload (Variable):** Dữ liệu thực tế (JSON chuỗi, hoặc binary struct).

### 22. Tại sao trường `Length` trong Header lại cần 2 bytes?
*   **Trả lời:** Để hỗ trợ gói tin có kích thước tối đa 65.535 bytes.
*   **Bản chất chi tiết:**
    *   1 byte chỉ biểu diễn được tối đa 255 (0-255). Nếu gói tin dài hơn 255 byte (ví dụ danh sách user online), 1 byte không đủ chứa độ dài.
    *   2 bytes (uint16_t) biểu diễn được từ 0 đến 65535. Đủ lớn cho hầu hết các message game thông thường.

### 23. Tại sao khi gửi `length` đi lại phải dùng `htons`, và khi nhận về lại dùng `ntohs`?
*   **Trả lời:** Đồng bộ hóa Endianness (Thứ tự byte) giữa các máy.
*   **Bản chất chi tiết:**
    *   Length là số 2 byte (uint16). Máy gửi có thể là Little Endian, máy nhận là Big Endian.
    *   Quy ước mạng là Big Endian.
    *   Gửi: Host -> Network (`htons`).
    *   Nhận: Network -> Host (`ntohs`).
    *   Nếu không làm vậy, gửi độ dài 256 (0x0100) có thể bị đọc thành 1 (0x0001).

### 24. Nếu `recv` trả về số byte ít hơn kích thước gói tin mong đợi, server xử lý ra sao?
*   **Trả lời:** Lưu tạm vào buffer và chờ lần `recv` tiếp theo.
*   **Bản chất chi tiết:**
    *   Trong hàm `receivePacket`, sau khi đọc Header và biết `Length` cần thiết.
    *   Nó kiểm tra `buffer.size() < 3 + Length`. Nếu đúng (thiếu dữ liệu), nó `return false` (hoặc break vòng lặp xử lý).
    *   Dữ liệu vẫn nằm yên trong buffer của client đó. Lần tới khi có thêm dữ liệu đến, buffer đầy thêm, vòng lặp kiểm tra lại sẽ thấy đủ và xử lý.

### 25. Việc sử dụng `std::vector<uint8_t>` cho payload có ưu điểm gì so với mảng C (`char[]`)?
*   **Trả lời:** Quản lý bộ nhớ tự động và an toàn hơn.
*   **Bản chất chi tiết:**
    *   `std::vector` tự động `delete` vùng nhớ khi ra khỏi phạm vi, tránh Memory Leak.
    *   Nó biết kích thước của mình (`.size()`), không cần truyền kèm biến đếm size như mảng C.
    *   Dễ dàng copy, nối mảng (`insert`), thay đổi kích thước động (`resize`) mà không cần `malloc`/`realloc` thủ công dễ gây lỗi.

### 26. Enum `MessageType` dùng `uint8_t` (1 byte) có đủ không?
*   **Trả lời:** Đủ cho 256 loại thông điệp.
*   **Bản chất chi tiết:**
    *   Game cờ vua này chỉ có khoảng 20-30 loại message (Login, Move, Start, End...).
    *   256 giá trị là quá dư giả. Dùng 1 byte giúp tiết kiệm băng thông header (Header chỉ tốn 3 byte tổng cộng).

---

## IV. Đa luồng & Đồng bộ hóa (Multithreading & Concurrency)

### 27. Tại sao cần phải dùng `std::mutex` (biến `clients_mutex`) trong `NetworkServer`?
*   **Trả lời:** Để tránh **Race Condition** (Điều kiện đua) khi truy cập tài nguyên chia sẻ.
*   **Bản chất chi tiết:**
    *   Biến `clients` (unordered_map) được truy cập bởi nhiều luồng: Luồng chính (accept thêm client mới), Luồng client A (đọc thông tin client B), Luồng client C (ngắt kết nối, xóa khỏi map).
    *   Nếu 2 luồng cùng ghi vào map cùng lúc, cấu trúc dữ liệu bên trong map có thể bị hỏng (corrupted), gây crash chương trình. Mutex đảm bảo tại 1 thời điểm chỉ có 1 luồng được đụng vào `clients`.

### 28. `std::lock_guard` hoạt động như thế nào và tại sao nên dùng nó thay vì `mutex.lock()` và `unlock()` thủ công?
*   **Trả lời:** Dùng cơ chế **RAII** để tự động mở khóa, tránh Deadlock.
*   **Bản chất chi tiết:**
    *   `std::lock_guard<std::mutex> lock(mutex);` -> Gọi `mutex.lock()` ngay khi khởi tạo.
    *   Khi biến `lock` bị hủy (do hàm kết thúc, hoặc do gặp lệnh `return`, hoặc do có Exception ném ra), destructor của nó sẽ tự động gọi `mutex.unlock()`.
    *   Nếu dùng thủ công, bạn rất dễ quên `unlock()` khi code có nhiều điểm `return`, dẫn đến việc các luồng khác chờ mãi mãi (Deadlock).

### 29. Trong hàm `receivePacket`, tại sao lại có 2 lần lock mutex khác nhau?
*   **Trả lời:** Kỹ thuật **Granular Locking** (Khóa hạt nhỏ) để tối ưu hiệu năng.
*   **Bản chất chi tiết:**
    *   Lần 1: Lock `clients_mutex` (toàn cục) chỉ để lấy tham chiếu tới đối tượng `ClientInfo` trong map. Lock này giữ rất nhanh rồi thả ngay.
    *   Lần 2: Lock `client.mutex` (riêng của từng client) để thao tác buffer của client đó.
    *   **Lợi ích:** Nếu client A đang xử lý buffer của nó, client B không bị chặn (vì client B dùng mutex B). Nếu dùng 1 lock toàn cục cho tất cả, client A xử lý gói tin nặng sẽ làm treo toàn bộ server, client B không gửi được gì.

### 30. Điều gì xảy ra nếu hai client cùng thách đấu một người thứ ba cùng lúc?
*   **Trả lời:** Yêu cầu nào đến trước (chiếm được lock trước) sẽ được xử lý trước.
*   **Bản chất chi tiết:**
    *   Giả sử A và B cùng gửi Challenge tới C.
    *   Server xử lý trên 2 luồng riêng biệt. Cả 2 luồng đều cố gắng truy cập trạng thái của C (ví dụ: `isUserInGame`).
    *   Nhờ Mutex bảo vệ logic game, luồng của A sẽ kiểm tra thấy C rảnh -> Set C bận -> Gửi lời mời. Luồng của B vào sau sẽ thấy C đã bận (hoặc đang có lời mời) -> Báo lỗi cho B.

### 31. Hàm `join()` trong `main()` có tác dụng gì?
*   **Trả lời:** Chờ cho luồng con kết thúc rồi mới cho luồng chính kết thúc.
*   **Bản chất chi tiết:**
    *   Nếu luồng chính (`main`) kết thúc trước, toàn bộ tiến trình bị hủy, các luồng con (đang xử lý client) sẽ bị giết đột ngột (terminate), có thể gây mất dữ liệu hoặc không đóng socket tử tế.
    *   `join()` chặn luồng chính lại, đảm bảo mọi việc dọn dẹp của luồng con hoàn tất. Tuy nhiên trong server này, `main` chạy vòng lặp vô tận nên `join` chỉ có tác dụng khi có tín hiệu dừng server.

### 32. Biến `static` trong hàm `getInstance()` của Singleton có đảm bảo thread-safe trong C++11 không?
*   **Trả lời:** Có, đây là tính năng **Magic Statics** của C++11.
*   **Bản chất chi tiết:**
    *   Trước C++11, khởi tạo biến static local trong môi trường đa luồng có thể bị Race Condition (2 luồng cùng khởi tạo 2 lần). Phải dùng Double-Checked Locking rất phức tạp.
    *   Từ C++11, trình biên dịch đảm bảo việc khởi tạo này là Thread-safe: Chỉ một luồng được khởi tạo, các luồng khác sẽ chờ cho đến khi khởi tạo xong.

---

## V. Logic Game & Nghiệp vụ (Game Logic)

### 33. Làm thế nào server xác định một người chơi đã đăng nhập hay chưa?
*   **Trả lời:** Kiểm tra trường `username` trong `ClientInfo`.
*   **Bản chất chi tiết:**
    *   Khi mới kết nối, `ClientInfo` được tạo với `username` rỗng.
    *   Khi xử lý gói tin `LOGIN_SUCCESS`, server gọi `setUsername` để cập nhật tên.
    *   Hàm `isUserLoggedIn` chỉ đơn giản kiểm tra xem có client nào trong map có `username` khớp và khác rỗng hay không.

### 34. Logic ghép cặp tự động (Auto Match) hoạt động như thế nào?
*   **Trả lời:** Sử dụng hàng đợi (Queue) FIFO.
*   **Bản chất chi tiết:**
    *   Khi user bấm "Tìm trận", user đó được đẩy vào `std::deque` hoặc `vector` hàng đợi.
    *   Server kiểm tra kích thước hàng đợi. Nếu `size >= 2`:
    *   Lấy người đầu tiên (A) và người thứ hai (B) ra khỏi hàng đợi.
    *   Tạo một `Game` mới cho A và B.
    *   Gửi thông báo `AUTO_MATCH_FOUND` cho cả hai.

### 35. Tại sao cần kiểm tra chênh lệch Rank (Elo) khi thách đấu?
*   **Trả lời:** Để đảm bảo cân bằng game (Game Balance) và trải nghiệm người dùng.
*   **Bản chất chi tiết:**
    *   Nếu một người Rank 1000 đấu với Rank 2000, kết quả gần như định sẵn. Người thấp chán nản, người cao không học được gì.
    *   Server chặn ngay từ tầng logic (`MessageHandler`): Tính `abs(rankA - rankB)`. Nếu > 10 (hoặc ngưỡng quy định), trả về lỗi ngay, không gửi lời mời đi.

### 36. Khi client A gửi nước đi (MOVE), server làm gì để chuyển nước đi đó cho client B?
*   **Trả lời:** Server đóng vai trò **Relay** (Trung chuyển) có kiểm soát.
*   **Bản chất chi tiết:**
    1.  Nhận gói `MOVE` từ A.
    2.  `GameManager` tìm trận đấu mà A đang tham gia.
    3.  Xác định đối thủ của A là B.
    4.  Validate nước đi (có đúng luật cờ vua không) trên bàn cờ ảo tại server.
    5.  Nếu đúng, cập nhật bàn cờ server.
    6.  Tìm `client_fd` của B dựa trên username.
    7.  Gửi gói tin `MOVE` (chứa nước đi của A) xuống cho B.

### 37. Server lưu trạng thái bàn cờ (FEN) để làm gì?
*   **Trả lời:** Để làm **Source of Truth** (Nguồn sự thật duy nhất) và chống gian lận.
*   **Bản chất chi tiết:**
    *   Nếu server không lưu bàn cờ, client A có thể gửi "Tôi đi Tốt ăn Vua", server cứ thế chuyển cho B -> Hack game.
    *   Server phải có bàn cờ riêng, tự chạy logic cờ vua để kiểm tra tính hợp lệ của mọi nước đi client gửi lên.
    *   Ngoài ra, FEN giúp khôi phục bàn cờ nếu client bị crash và kết nối lại (Reconnect).

### 38. Xử lý thế nào khi một người chơi bị mất kết nối giữa ván cờ?
*   **Trả lời:** Xử phạt người thoát, xử thắng cho người ở lại.
*   **Bản chất chi tiết:**
    *   Khi `recv` báo lỗi hoặc = 0 (disconnect).
    *   `GameManager` nhận sự kiện `clientDisconnected`.
    *   Kiểm tra xem user đó có đang trong game không.
    *   Nếu có, kết thúc game đó ngay lập tức. User thoát bị tính thua (hoặc trừ điểm uy tín), user còn lại được tính thắng (do đối thủ bỏ cuộc). Gửi thông báo `GAME_END` cho người còn lại.

---

## VI. C++ & Kỹ thuật Code (C++ Specifics)

### 39. `std::vector::data()` trả về cái gì? Tại sao dùng nó trong hàm `send`?
*   **Trả lời:** Trả về con trỏ trỏ tới mảng dữ liệu thô (`raw pointer`).
*   **Bản chất chi tiết:**
    *   `std::vector` bọc một mảng động bên trong.
    *   Hàm hệ thống `send()` của C (socket API) không hiểu `std::vector` là gì, nó chỉ nhận `const void* buffer` và `size_t length`.
    *   `.data()` cung cấp đúng cái `send` cần: địa chỉ bắt đầu của vùng nhớ chứa dữ liệu.

### 40. Tại sao dùng `const std::vector<uint8_t> &payload` (tham chiếu hằng) trong các hàm xử lý?
*   **Trả lời:** Tối ưu hiệu năng và đảm bảo an toàn dữ liệu.
*   **Bản chất chi tiết:**
    *   **Tham chiếu (`&`):** Truyền địa chỉ của vector thay vì copy toàn bộ mảng dữ liệu sang một vùng nhớ mới. Nếu payload nặng 1MB, việc copy sẽ rất chậm.
    *   **Hằng (`const`):** Cam kết rằng hàm này chỉ ĐỌC dữ liệu, không sửa đổi nó. Giúp code an toàn hơn, tránh lỗi logic vô tình sửa đổi dữ liệu gốc.

### 41. `static_cast` khác gì với ép kiểu kiểu C `(int)variable`?
*   **Trả lời:** An toàn hơn và rõ ràng hơn.
*   **Bản chất chi tiết:**
    *   Ép kiểu C `(type)val` rất mạnh bạo, nó có thể ép con trỏ thành số, ép const thành non-const... dễ gây lỗi runtime.
    *   `static_cast` kiểm tra tính tương thích tại thời điểm biên dịch (Compile-time). Ví dụ: Nó cho phép chuyển `int` sang `float`, `enum` sang `int`, nhưng sẽ báo lỗi nếu bạn cố chuyển con trỏ của 2 class không liên quan nhau.

### 42. `std::shared_ptr` (nếu có dùng) giúp ích gì trong việc quản lý Game/Room?
*   **Trả lời:** Quản lý vòng đời đối tượng tự động bằng **Reference Counting** (Đếm tham chiếu).
*   **Bản chất chi tiết:**
    *   Một ván cờ (`Game`) có thể được tham chiếu bởi `GameManager`, và bởi 2 `ClientInfo` của 2 người chơi.
    *   Nếu dùng con trỏ thường, khi nào thì `delete Game`? Khi A thoát? Lỡ B vẫn đang xem lại ván đấu thì sao?
    *   `shared_ptr` đếm số người đang giữ nó. Khi cả A, B và Manager đều buông bỏ (hủy pointer), `count` về 0 -> Đối tượng `Game` tự động được giải phóng. Không bao giờ lo Memory Leak hay Dangling Pointer.

### 43. Tại sao `NetworkServer` lại xóa Copy Constructor và Assignment Operator (`= delete`)?
*   **Trả lời:** Để bảo vệ tính chất Singleton.
*   **Bản chất chi tiết:**
    *   Mặc định C++ tự sinh code cho phép copy object: `NetworkServer s2 = s1;`.
    *   Nếu điều này xảy ra, ta sẽ có 2 object server, có thể dẫn đến việc đóng nhầm socket của nhau (Double close) hoặc trạng thái không đồng nhất.
    *   `= delete` báo cho compiler biết: "Cấm tiệt việc copy class này. Nếu ai cố tình viết code copy, hãy báo lỗi biên dịch ngay".

### 44. `perror()` dùng để làm gì?
*   **Trả lời:** In thông báo lỗi hệ thống một cách dễ hiểu.
*   **Bản chất chi tiết:**
    *   Các hàm hệ thống (socket, bind, accept) khi lỗi thường trả về -1 và đặt một mã số lỗi vào biến toàn cục `errno`.
    *   `perror("bind failed")` sẽ tra cứu mã `errno` đó và in ra câu tiếng Anh tương ứng.
    *   Ví dụ: Thay vì chỉ in "bind failed", nó sẽ in "bind failed: Address already in use" hoặc "bind failed: Permission denied", giúp lập trình viên biết chính xác nguyên nhân.

### 45. `memset` dùng để làm gì khi khởi tạo `sockaddr_in`?
*   **Trả lời:** Xóa sạch vùng nhớ về 0 (Zeroing out).
*   **Bản chất chi tiết:**
    *   Struct `sockaddr_in` có một phần đệm (padding) gọi là `sin_zero` để kích thước nó bằng với `struct sockaddr` chuẩn.
    *   Nếu không set về 0, phần này chứa rác (garbage value). Một số hệ thống nghiêm ngặt có thể từ chối bind nếu cấu trúc chứa rác. `memset` đảm bảo cấu trúc sạch sẽ hoàn toàn trước khi thiết lập các trường IP, Port.

---

## VII. Mở rộng & Nâng cao (Advanced & Scaling)

### 46. Nếu muốn server hỗ trợ 10.000 kết nối đồng thời (C10K), bạn sẽ thay đổi gì?
*   **Trả lời:** Chuyển sang mô hình **Non-blocking I/O** kết hợp **I/O Multiplexing**.
*   **Bản chất chi tiết:**
    *   Thay vì tạo 10.000 thread (chết chắc), ta chỉ dùng 1 (hoặc một vài) thread duy nhất.
    *   Sử dụng các system call như **`epoll`** (Linux) hoặc **`kqueue`** (MacOS), **`IOCP`** (Windows).
    *   Cơ chế: "Này hệ điều hành, hãy canh chừng 10.000 cái socket này giúp tôi. Khi nào cái nào CÓ DỮ LIỆU ĐẾN thì báo tôi biết".
    *   Thread chính chỉ việc ngủ, khi tỉnh dậy nó nhận được danh sách "Các socket đang có dữ liệu", nó xử lý lần lượt từng cái rất nhanh rồi lại ngủ tiếp. Không tốn RAM cho stack của thread.

### 47. Làm sao để bảo mật password người dùng khi gửi qua mạng?
*   **Trả lời:** Không bao giờ gửi Plaintext. Sử dụng **Hashing** và **Encryption**.
*   **Bản chất chi tiết:**
    *   **Client side:** Trước khi gửi, client nên Hash password (SHA256) hoặc tốt hơn là gửi qua kênh truyền mã hóa.
    *   **Network Layer:** Sử dụng **SSL/TLS** (bọc socket thường thành secure socket - giống HTTPS). Thư viện OpenSSL thường được dùng. Khi đó mọi gói tin (kể cả password) đều được mã hóa, hacker bắt gói tin chỉ thấy rác.
    *   **Server side:** Không lưu password gốc vào database/file. Chỉ lưu Hash (kèm Salt) để so sánh.

### 48. Làm thế nào để xử lý việc client gửi spam packet liên tục (DDoS)?
*   **Trả lời:** Áp dụng **Rate Limiting** (Giới hạn tốc độ).
*   **Bản chất chi tiết:**
    *   Trong `ClientInfo`, thêm biến đếm `message_count` và `last_message_time`.
    *   Mỗi khi nhận gói tin, kiểm tra: "Trong 1 giây qua, anh này gửi bao nhiêu gói?".
    *   Nếu > 10 gói/giây -> Nghi vấn spam -> Drop gói tin hoặc Disconnect luôn.
    *   Kiểm tra kích thước gói tin: Nếu Header báo Length = 1GB -> Từ chối ngay, không cấp phát bộ nhớ để tránh tràn RAM (Memory Exhaustion Attack).

### 49. Tại sao nên dùng `uint16_t`, `uint8_t` thay vì `int`, `short` trong protocol?
*   **Trả lời:** Để đảm bảo **Fixed Width** (Độ rộng cố định) đa nền tảng.
*   **Bản chất chi tiết:**
    *   Kiểu `int` trên máy này có thể là 2 byte, máy kia là 4 byte, máy nọ là 8 byte.
    *   Giao thức mạng cần sự chính xác tuyệt đối từng bit. `uint16_t` luôn luôn là 16 bit (2 byte) bất kể chạy trên Arduino, PC hay Supercomputer. Điều này đảm bảo struct `Packet` có kích thước đồng nhất giữa Client và Server.

### 50. Cơ chế "Heartbeat" (Keep-alive) có tác dụng gì trong game online?
*   **Trả lời:** Phát hiện kết nối chết (Dead Connection) và giữ NAT mở.
*   **Bản chất chi tiết:**
    *   **Vấn đề:** Nếu ai đó rút dây mạng (hoặc mất điện), máy tính không kịp gửi gói FIN. Server vẫn tưởng client đang online (socket vẫn mở), lãng phí tài nguyên.
    *   **Giải pháp:** Cứ 5 giây, client gửi 1 gói tin nhỏ "Ping". Server nhận được trả lời "Pong".
    *   Nếu server quá 15 giây không thấy "Ping" nào từ client -> Kết luận client đã mất mạng -> Chủ động đóng kết nối (Time out).

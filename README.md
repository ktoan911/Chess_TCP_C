# TCP Chess

TCP Chess là một ứng dụng trò chơi cờ vua đa người chơi sử dụng giao thức TCP/IP. Ứng dụng này cho phép nhiều người chơi tham gia trò chơi cờ vua trực tuyến thông qua mạng.

## Hướng Dẫn Chạy Project TCP Chess

### Yêu Cầu Hệ Thống
- Linux
- C++17
- Make

### Cài Đặt

**Biên Dịch Sử Dụng Makefile:**
   ```bash
   make all
   ```

Các tệp thực thi sẽ được tạo trong thư mục build.

### Chạy Project

#### Sử dụng Makefile

1. **Chạy Server:**
   ```bash
   make run_server
   ```

2. **Chạy Client:**
   Mở một terminal mới cho mỗi client và chạy:
   ```bash
   make run_client
   ```

#### Chạy Thẳng Các Tệp Thực Thi

1. **Chạy Server:**
   ```bash
   ./build/server_main
   ```

2. **Chạy Client:**
   Mở một terminal mới cho mỗi client và chạy:
   ```bash
   ./build/client_main
   ```

### Lưu Ý
- **Chạy Server Trước Các Client:** Đảm bảo rằng server đang chạy trước khi khởi động bất kỳ client nào.
- **Chạy Nhiều Client:** Bạn có thể chạy nhiều phiên bản client đồng thời bằng cách mở nhiều terminal và thực hiện lệnh chạy client trong mỗi terminal.

### Dọn Dẹp
Để xóa các tệp biên dịch:
```bash
make clean
```
### Dữ liệu mẫu
- Các username mẫu để demo: `vu1`, `vu2`, `viet1`, `viet2`

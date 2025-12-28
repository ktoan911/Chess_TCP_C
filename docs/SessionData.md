# SessionData - Chi Tiết và Ví Dụ Sử Dụng

## Tổng Quan

`SessionData` là một singleton class trong file `client/session_data.hpp`, được sử dụng để quản lý dữ liệu phiên làm việc của người dùng trong client. Nó lưu trữ thông tin như tên người dùng, điểm ELO, và trạng thái trò chơi hiện tại. Được thiết kế cho kiến trúc single-threaded.

## Cấu Trúc và Thành Phần

### 1. Singleton Pattern
- `SessionData` sử dụng pattern Singleton để đảm bảo chỉ có một instance duy nhất trong toàn bộ chương trình.
- Truy cập thông qua `SessionData::getInstance()`.

### 2. Dữ Liệu Cơ Bản
- `std::string username_`: Tên người dùng đã đăng nhập.
- `uint16_t elo_`: Điểm ELO của người dùng.

### 3. Trạng Thái Game (GameStatus struct)
- `std::string game_id`: ID của trận đấu hiện tại.
- `bool is_my_turn`: Có phải lượt của người dùng hay không.
- `bool is_white`: Người dùng có chơi quân trắng hay không.
- `std::string fen`: Chuỗi FEN biểu diễn trạng thái bàn cờ.

### 4. Phương Thức Chính

#### Getter/Setter cho Username và ELO
- `const std::string& getUsername() const`
- `void setUsername(const std::string& username)`
- `uint16_t getElo() const`
- `void setElo(uint16_t elo)`

#### Quản Lý Trạng Thái Game
- `void setGameStatus(const std::string& game_id, bool is_white, const std::string& fen)`: Thiết lập trạng thái game mới.
- `void clearGameStatus()`: Xóa trạng thái game (kết thúc trận).
- `void setTurn(bool is_my_turn)`: Cập nhật lượt chơi.
- `void setFen(const std::string& fen)`: Cập nhật vị trí bàn cờ.

#### Kiểm Tra Trạng Thái
- `bool isMyTurn() const`: Kiểm tra có phải lượt của người dùng.
- `bool isWhite() const`: Kiểm tra người dùng chơi quân trắng.
- `const std::string& getFen() const`: Lấy chuỗi FEN hiện tại.
- `const std::string& getGameId() const`: Lấy ID game hiện tại.
- `bool isInGame() const`: Kiểm tra có đang trong trận đấu hay không.

## Ví Dụ Sử Dụng Cụ Thể

### Ví Dụ 1: Thiết Lập Thông Tin Người Dùng Sau Đăng Nhập
Khi server gửi thông báo đăng nhập thành công, thông tin được lưu vào SessionData:

```cpp
// Trong message_handler.hpp, handleLoginSuccess
SessionData &session = SessionData::getInstance();
session.setUsername(message.username);
session.setElo(message.elo);
```

Tương tự cho đăng ký thành công:

```cpp
// Trong message_handler.hpp, handleRegisterSuccess
SessionData &session = SessionData::getInstance();
session.setUsername(message.username);
session.setElo(message.elo);
```

### Ví Dụ 2: Cập Nhật Trạng Thái Game
Khi bắt đầu trận đấu, thiết lập trạng thái game:

```cpp
// Trong message_handler.hpp, khi nhận GAME_START
session.setGameStatus(message.game_id, message.is_white, message.fen);
```

Khi nhận cập nhật trạng thái game:

```cpp
// Trong message_handler.hpp, handleGameStatusUpdate
SessionData &session = SessionData::getInstance();

session.setFen(message.fen);
bool is_my_turn = (message.current_turn_username == session.getUsername());
session.setTurn(is_my_turn);
```

### Ví Dụ 3: Sử Dụng Trong Input Processor
Trong input_processor.hpp, SessionData được sử dụng để lấy thông tin người dùng:

```cpp
SessionData& session_ = SessionData::getInstance();

// Khi gửi yêu cầu đăng ký
msg.username = session_.getUsername();

// Khi hiển thị danh sách người chơi
UI::displayPlayerList(context.player_list_cache, session_.getUsername());

// Khi kiểm tra tên người dùng
if (input == session_.getUsername()) {
    // Không thể thách đấu chính mình
}
```

### Ví Dụ 4: Gửi Move Trong Game
Khi người dùng nhập nước đi:

```cpp
// Trong input_processor.hpp
msg.game_id = session_.getGameId();
msg.from_username = session_.getUsername();
// ... gửi nước đi
```

### Ví Dụ 5: Kiểm Tra Lượt Chơi
Trong UI hoặc logic hiển thị:

```cpp
SessionData& session = SessionData::getInstance();
if (session.isMyTurn()) {
    // Hiển thị prompt nhập nước đi
} else {
    // Hiển thị chờ đối thủ
}
```

## Cách Hoạt Động

- **Singleton Access**: Luôn truy cập qua `SessionData::getInstance()` để đảm bảo consistency.
- **Thread Safety**: Được thiết kế cho single-threaded, không có synchronization.
- **Persistence**: Dữ liệu tồn tại suốt phiên làm việc của client, bị reset khi thoát.
- **Integration**: Được sử dụng trong cả MessageHandler và InputProcessor để chia sẻ trạng thái.

## Lợi Ích

- **Centralized State**: Tập trung quản lý trạng thái người dùng.
- **Easy Access**: Dễ dàng truy cập từ bất kỳ đâu trong client code.
- **Memory Efficient**: Chỉ một instance duy nhất.
- **Clean API**: Getter/setter rõ ràng cho từng loại dữ liệu.</content>
<parameter name="filePath">/home/linh/ltm/Chess_TCP_C/docs/SessionData.md
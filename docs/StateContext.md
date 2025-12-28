# StateContext - Chi Tiết và Ví Dụ Sử Dụng

## Tổng Quan

`StateContext` là một struct trong file `client/client_state.hpp`, được sử dụng để lưu trữ dữ liệu ngữ cảnh tạm thời giữa các trạng thái (states) trong máy trạng thái của client. Nó không dùng để lưu trữ lâu dài mà chỉ phục vụ việc truyền và duy trì trạng thái dữ liệu trong quá trình hoạt động của client.

## Cấu Trúc và Thành Phần

`StateContext` bao gồm các biến thành viên sau:

### 1. Auto Match Data (Dữ liệu cho chế độ tìm trận tự động)
- `std::string pending_game_id`: Lưu trữ ID của trận đấu đang chờ.
- `std::string opponent_username`: Lưu tên người dùng của đối thủ.
- `uint16_t opponent_elo`: Lưu điểm ELO của đối thủ.

### 2. Challenge Data (Dữ liệu cho chế độ thách đấu)
- `std::string challenger_username`: Lưu tên người dùng của người thách đấu.
- `uint16_t challenger_elo`: Lưu điểm ELO của người thách đấu.

### 3. Player List Cache (Cache danh sách người chơi)
- `std::vector<PlayerListMessage::Player> player_list_cache`: Lưu trữ danh sách người chơi trực tuyến.

### 4. Timeout Tracking (Theo dõi thời gian chờ)
- `int timeout_counter`: Bộ đếm thời gian chờ cho các trạng thái cần timeout.

### 5. Phương Thức
- `void clear()`: Đặt lại tất cả các biến thành viên về giá trị mặc định.

## Ví Dụ Sử Dụng Cụ Thể

### Ví Dụ 1: Xử Lý Auto Match
Khi server gửi thông báo tìm thấy trận đấu tự động, `StateContext` được cập nhật trong `message_handler.hpp`:

```cpp
// Trong message_handler.hpp, khi nhận AUTO_MATCH_FOUND
context.pending_game_id = message.game_id;
context.opponent_username = message.opponent_username;
context.opponent_elo = message.opponent_elo;
```

Sau đó, trong `input_processor.hpp`, thông tin này được sử dụng để hiển thị prompt cho người dùng:

```cpp
// Trong input_processor.hpp, khi ở trạng thái AUTO_MATCH_DECISION
UI::displayAutoMatchOptionsPrompt(context.opponent_username, 
                                  context.opponent_elo, 
                                  context.pending_game_id);
```

Khi người dùng chấp nhận, ID trận được sử dụng để gửi phản hồi:

```cpp
msg.game_id = context.pending_game_id;
```

### Ví Dụ 2: Xử Lý Challenge
Khi nhận được lời thách đấu, thông tin được lưu vào context:

```cpp
// Trong message_handler.hpp
context.challenger_username = message.from_username;
context.challenger_elo = message.elo;
```

Trong input processor, hiển thị prompt và xử lý phản hồi:

```cpp
UI::displayChallengeDecisionPrompt(context.challenger_username, context.challenger_elo);
```

Khi chấp nhận, gửi phản hồi với tên người thách đấu:

```cpp
msg.from_username = context.challenger_username;
```

### Ví Dụ 3: Cache Player List
Khi nhận danh sách người chơi từ server:

```cpp
// Trong message_handler.hpp
context.player_list_cache = message.players;
```

Sau đó, hiển thị danh sách:

```cpp
UI::displayPlayerList(context.player_list_cache, session_.getUsername());
```

Và duyệt qua danh sách để chọn người chơi:

```cpp
for (const auto &player : context.player_list_cache) {
    // Xử lý từng người chơi
}
```

### Ví Dụ 4: Clear Context
Sau khi hoàn thành một hành động, context được xóa để chuẩn bị cho trạng thái mới:

```cpp
// Sau khi chấp nhận hoặc từ chối auto match
context.clear();
```

## Cách Hoạt Động Trong Main Loop

Trong `client_main.cpp`, `StateContext` được khai báo và truyền vào các hàm xử lý:

```cpp
StateContext context;

// Trong vòng lặp chính
currentState = inputProcessor.processInput(currentState, inputBuffer, context);
ClientState newState = messageHandler.handleMessage(currentState, packet, context);
```

Điều này cho phép dữ liệu được duy trì và chia sẻ giữa các module xử lý input và message mà không cần global variables.

## Lợi Ích

- **Tách biệt logic**: Dữ liệu tạm thời được tách riêng khỏi logic trạng thái.
- **Dễ bảo trì**: Tập trung hóa việc quản lý dữ liệu ngữ cảnh.
- **Tránh rò rỉ**: Phương thức `clear()` đảm bảo dữ liệu cũ không còn sót lại.
- **Hiệu suất**: Cache player list giảm số lần request đến server.</content>
<parameter name="filePath">/home/linh/ltm/Chess_TCP_C/docs/StateContext.md
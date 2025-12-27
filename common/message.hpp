// Header guard - Ngăn chặn việc include file này nhiều lần trong quá trình biên dịch
#ifndef MESSAGE_HPP
#define MESSAGE_HPP

// Include các thư viện cần thiết
#include <string>       // Thư viện xử lý chuỗi ký tự
#include <vector>       // Thư viện xử lý mảng động
#include <memory>       // Thư viện quản lý bộ nhớ (smart pointers)

#include "utils.hpp"    // File chứa các hàm tiện ích
#include "protocol.hpp" // File định nghĩa giao thức truyền thông
#include <stdexcept>    // Thư viện xử lý ngoại lệ

// Định nghĩa độ dài tối đa cho một trường dữ liệu là 255 bytes
constexpr size_t MAX_FIELD_LENGTH = 255;

// Hàm kiểm tra xem payload có đủ dữ liệu để đọc hay không
// payload: dữ liệu cần kiểm tra
// pos: vị trí hiện tại trong payload
// n: số byte cần đọc
inline void ensure_available(const std::vector<uint8_t>& payload, size_t pos, size_t n)
{
    // Nếu vị trí hiện tại + số byte cần đọc > kích thước payload
    // thì ném ra lỗi "payload quá nhỏ"
    if (pos + n > payload.size())
        throw std::runtime_error("payload too small");
}

// Hàm đọc 1 byte (uint8_t) từ payload
// Tham số pos được truyền bằng tham chiếu và sẽ được tăng lên sau khi đọc
inline uint8_t read_u8(const std::vector<uint8_t>& payload, size_t &pos)
{
    // Đảm bảo còn đủ 1 byte để đọc
    ensure_available(payload, pos, 1);
    // Đọc byte tại vị trí pos và tăng pos lên 1
    return payload[pos++];
}

// Hàm đọc 2 bytes (uint16_t) từ payload theo định dạng Big Endian
// Big Endian: byte cao được lưu trước
inline uint16_t read_u16_be(const std::vector<uint8_t>& payload, size_t &pos)
{
    // Đảm bảo còn đủ 2 bytes để đọc
    ensure_available(payload, pos, 2);
    // Chuyển đổi 2 bytes từ Big Endian sang số nguyên
    uint16_t v = from_big_endian_16(payload, pos);
    // Tăng vị trí lên 2 bytes
    pos += 2;
    return v;
}

// Hàm đọc 8 bytes (int64_t) từ payload theo định dạng Big Endian
inline int64_t read_i64_be(const std::vector<uint8_t>& payload, size_t &pos)
{
    // Đảm bảo còn đủ 8 bytes để đọc
    ensure_available(payload, pos, 8);
    // Khởi tạo giá trị kết quả
    int64_t v = 0;
    // Lặp qua 8 bytes
    for (int i = 0; i < 8; ++i)
    {
        // Dịch bit sang trái 8 vị trí và OR với byte hiện tại
        // Đây là cách chuyển đổi từ Big Endian
        v = (v << 8) | payload[pos++];
    }
    return v;
}

// Hàm đọc một chuỗi từ payload
// Chuỗi được lưu dưới dạng: 1 byte chỉ độ dài + các byte dữ liệu
inline std::string read_string(const std::vector<uint8_t>& payload, size_t &pos)
{
    // Đọc byte đầu tiên là độ dài của chuỗi
    uint8_t length = read_u8(payload, pos);
    // Kiểm tra độ dài có vượt quá giới hạn không
    if (length > MAX_FIELD_LENGTH)
        throw std::runtime_error("field length too large");
    // Đảm bảo còn đủ bytes để đọc chuỗi
    ensure_available(payload, pos, length);
    // Tạo chuỗi từ các bytes trong payload
    std::string s(payload.begin() + pos, payload.begin() + pos + length);
    // Tăng vị trí lên số byte đã đọc
    pos += length;
    return s;
}

#pragma region RegisterMessage 
// ===== MESSAGE ĐĂNG KÝ TÀI KHOẢN =====
// Được gửi từ client đến server để đăng ký người dùng mới
/*
Cấu trúc Payload:
    - uint8_t username_length (1 byte): Độ dài tên người dùng
    - char[username_length] username: Tên người dùng
*/
struct RegisterMessage
{
    std::string username;  // Tên người dùng muốn đăng ký

    // Hàm lấy loại message này
    MessageType getType() const
    {
        return MessageType::REGISTER;
    }

    // Hàm chuyển đổi message thành bytes để gửi qua mạng
    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;  // Vector chứa dữ liệu

        // Thêm độ dài username vào đầu (1 byte)
        payload.push_back(static_cast<uint8_t>(username.size()));
        // Thêm các ký tự của username vào sau
        payload.insert(payload.end(), username.begin(), username.end());

        return payload;
    }

    // Hàm chuyển đổi bytes nhận được thành RegisterMessage object
    static RegisterMessage deserialize(const std::vector<uint8_t> &payload)
    {
        RegisterMessage message;
        size_t pos = 0;  // Vị trí đọc bắt đầu từ 0
        // Đọc username từ payload
        message.username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion RegisterMessage

#pragma region RegisterSuccessMessage 
// ===== MESSAGE ĐĂNG KÝ THÀNH CÔNG =====
// Được gửi từ server về client để thông báo đăng ký thành công
/*
Cấu trúc Payload:
    - uint8_t username_length (1 byte): Độ dài tên người dùng
    - char[username_length] username: Tên người dùng
    - uint16_t elo (2 bytes): Điểm Elo của người chơi
*/
struct RegisterSuccessMessage
{
    std::string username;  // Tên người dùng đã đăng ký
    uint16_t elo;         // Điểm Elo ban đầu

    MessageType getType() const
    {
        return MessageType::REGISTER_SUCCESS;
    }

    // Hàm serialize để gửi dữ liệu
    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm độ dài và nội dung username
        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        // Chuyển điểm Elo sang Big Endian (2 bytes) và thêm vào payload
        std::vector<uint8_t> elo_bytes = to_big_endian_16(elo);
        payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());

        return payload;
    }

    // Hàm deserialize để nhận dữ liệu
    static RegisterSuccessMessage deserialize(const std::vector<uint8_t> &payload)
    {
        RegisterSuccessMessage message;
        size_t pos = 0;
        // Đọc username
        message.username = read_string(payload, pos);
        // Đọc điểm Elo (2 bytes, Big Endian)
        message.elo = read_u16_be(payload, pos);
        return message;
    }
};
#pragma endregion RegisterSuccessMessage

#pragma region RegisterFailureMessage 
// ===== MESSAGE ĐĂNG KÝ THẤT BẠI =====
// Được gửi từ server về client để thông báo đăng ký thất bại
/*
Cấu trúc Payload:
    - uint8_t error_message_length (1 byte): Độ dài thông báo lỗi
    - char[error_message_length] error_message: Nội dung thông báo lỗi
*/
struct RegisterFailureMessage
{
    std::string error_message;  // Thông báo lỗi (ví dụ: "Tên đã tồn tại")

    MessageType getType() const
    {
        return MessageType::REGISTER_FAILURE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm độ dài và nội dung thông báo lỗi
        payload.push_back(static_cast<uint8_t>(error_message.size()));
        payload.insert(payload.end(), error_message.begin(), error_message.end());

        return payload;
    }

    static RegisterFailureMessage deserialize(const std::vector<uint8_t> &payload)
    {
        RegisterFailureMessage message;
        size_t pos = 0;
        message.error_message = read_string(payload, pos);
        return message;
    }
};
#pragma endregion RegisterFailureMessage

#pragma region LoginMessage 
// ===== MESSAGE ĐĂNG NHẬP =====
// Được gửi từ client đến server để đăng nhập
/*
Cấu trúc Payload:
    - uint8_t username_length (1 byte): Độ dài tên người dùng
    - char[username_length] username: Tên người dùng
*/
struct LoginMessage
{
    std::string username;  // Tên người dùng muốn đăng nhập

    MessageType getType() const
    {
        return MessageType::LOGIN;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        return payload;
    }

    static LoginMessage deserialize(const std::vector<uint8_t> &payload)
    {
        LoginMessage message;
        size_t pos = 0;
        message.username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion LoginMessage

#pragma region LoginSuccessMessage 
// ===== MESSAGE ĐĂNG NHẬP THÀNH CÔNG =====
// Được gửi từ server về client để thông báo đăng nhập thành công
/*
Cấu trúc Payload:
    - uint8_t username_length (1 byte): Độ dài tên người dùng
    - char[username_length] username: Tên người dùng
    - uint16_t elo (2 bytes): Điểm Elo hiện tại
*/
struct LoginSuccessMessage
{
    std::string username;  // Tên người dùng đã đăng nhập
    uint16_t elo;         // Điểm Elo hiện tại

    MessageType getType() const
    {
        return MessageType::LOGIN_SUCCESS;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;
        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        std::vector<uint8_t> elo_bytes = to_big_endian_16(elo);
        payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());
        return payload;
    }

    static LoginSuccessMessage deserialize(const std::vector<uint8_t> &payload)
    {
        LoginSuccessMessage message;
        size_t pos = 0;
        message.username = read_string(payload, pos);
        message.elo = read_u16_be(payload, pos);
        return message;
    }
};

#pragma region LoginFailureMessage 
// ===== MESSAGE ĐĂNG NHẬP THẤT BẠI =====
// Được gửi từ server về client để thông báo đăng nhập thất bại
/*
Cấu trúc Payload:
    - uint8_t error_message_length (1 byte): Độ dài thông báo lỗi
    - char[error_message_length] error_message: Nội dung thông báo lỗi
*/
struct LoginFailureMessage
{
    std::string error_message;  // Thông báo lỗi (ví dụ: "Tài khoản không tồn tại")

    MessageType getType() const
    {
        return MessageType::LOGIN_FAILURE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(error_message.size()));
        payload.insert(payload.end(), error_message.begin(), error_message.end());

        return payload;
    }

    static LoginFailureMessage deserialize(const std::vector<uint8_t> &payload)
    {
        LoginFailureMessage message;
        size_t pos = 0;
        message.error_message = read_string(payload, pos);
        return message;
    }
};
#pragma endregion LoginFailureMessage

#pragma region GameStartMessage 
// ===== MESSAGE BẮT ĐẦU TRÒ CHƠI =====
// Được gửi từ server đến cả 2 client để thông báo ván cờ bắt đầu
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
    - uint8_t player1_username_length (1 byte): Độ dài tên người chơi 1
    - char[player1_username_length] player1_username: Tên người chơi 1
    - uint8_t player2_username_length (1 byte): Độ dài tên người chơi 2
    - char[player2_username_length] player2_username: Tên người chơi 2
    - uint8_t starting_player_username_length (1 byte): Độ dài tên người đi trước
    - char[starting_player_username_length] starting_player_username: Tên người đi trước
    - uint8_t fen_length (1 byte): Độ dài chuỗi FEN
    - char[fen_length] fen: Chuỗi FEN mô tả trạng thái bàn cờ
*/
struct GameStartMessage
{
    std::string game_id;                  // ID định danh ván cờ
    std::string player1_username;         // Tên người chơi 1 (quân trắng)
    std::string player2_username;         // Tên người chơi 2 (quân đen)
    std::string starting_player_username; // Tên người được đi trước
    std::string fen;                      // FEN notation - mô tả trạng thái bàn cờ

    MessageType getType() const
    {
        return MessageType::GAME_START;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        // Thêm player1_username
        payload.push_back(static_cast<uint8_t>(player1_username.size()));
        payload.insert(payload.end(), player1_username.begin(), player1_username.end());

        // Thêm player2_username
        payload.push_back(static_cast<uint8_t>(player2_username.size()));
        payload.insert(payload.end(), player2_username.begin(), player2_username.end());

        // Thêm starting_player_username
        payload.push_back(static_cast<uint8_t>(starting_player_username.size()));
        payload.insert(payload.end(), starting_player_username.begin(), starting_player_username.end());

        // Thêm FEN notation
        payload.push_back(static_cast<uint8_t>(fen.size()));
        payload.insert(payload.end(), fen.begin(), fen.end());

        return payload;
    }

    static GameStartMessage deserialize(const std::vector<uint8_t> &payload)
    {
        GameStartMessage message;
        size_t pos = 0;
        // Đọc lần lượt từng trường
        message.game_id = read_string(payload, pos);
        message.player1_username = read_string(payload, pos);
        message.player2_username = read_string(payload, pos);
        message.starting_player_username = read_string(payload, pos);
        message.fen = read_string(payload, pos);
        return message;
    }
};
#pragma endregion GameStartMessage

#pragma region MoveMessage 
// ===== MESSAGE THỰC HIỆN NƯỚC ĐI =====
// Được gửi từ client đến server để thực hiện một nước đi
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
    - uint8_t uci_move_length (1 byte): Độ dài nước đi UCI
    - char[uci_move_length] uci_move: Nước đi theo định dạng UCI (ví dụ: "e2e4")
*/
struct MoveMessage
{
    std::string game_id;   // ID ván cờ
    std::string uci_move;  // Nước đi theo định dạng UCI (Universal Chess Interface)

    MessageType getType() const
    {
        return MessageType::MOVE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        // Thêm uci_move
        payload.push_back(static_cast<uint8_t>(uci_move.size()));
        payload.insert(payload.end(), uci_move.begin(), uci_move.end());

        return payload;
    }

    static MoveMessage deserialize(const std::vector<uint8_t> &payload)
    {
        MoveMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.uci_move = read_string(payload, pos);
        return message;
    }
};
#pragma endregion MoveMessage

#pragma region InvalidMoveMessage 
// ===== MESSAGE NƯỚC ĐI KHÔNG HỢP LỆ =====
// Được gửi từ server về client để thông báo nước đi không hợp lệ
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
    - uint8_t error_message_length (1 byte): Độ dài thông báo lỗi
    - char[error_message_length] error_message: Nội dung thông báo lỗi
*/
struct InvalidMoveMessage
{
    std::string game_id;        // ID ván cờ
    std::string error_message;  // Lý do nước đi không hợp lệ

    MessageType getType() const
    {
        return MessageType::INVALID_MOVE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        // Thêm thông báo lỗi
        payload.push_back(static_cast<uint8_t>(error_message.size()));
        payload.insert(payload.end(), error_message.begin(), error_message.end());

        return payload;
    }

    static InvalidMoveMessage deserialize(const std::vector<uint8_t> &payload)
    {
        InvalidMoveMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.error_message = read_string(payload, pos);
        return message;
    }
};
#pragma endregion InvalidMoveMessage

#pragma region GameStatusUpdateMessage 
// ===== MESSAGE CẬP NHẬT TRẠNG THÁI TRÒ CHƠI =====
// Được gửi từ server đến cả 2 client để cập nhật trạng thái ván cờ
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
    - uint8_t fen_length (1 byte): Độ dài chuỗi FEN
    - char[fen_length] fen: Chuỗi FEN mô tả trạng thái bàn cờ hiện tại
    - uint8_t current_turn_username_length (1 byte): Độ dài tên người đi tiếp theo
    - char[current_turn_username_length] current_turn_username: Tên người đi tiếp theo
    - uint8_t is_game_over (1 byte): Cờ báo ván cờ kết thúc (0: chưa, 1: đã kết thúc)
    - uint8_t message_length (1 byte): Độ dài thông báo
    - char[message_length] message: Nội dung thông báo
*/
struct GameStatusUpdateMessage
{
    std::string game_id;               // ID ván cờ
    std::string fen;                   // Trạng thái bàn cờ hiện tại (FEN)
    std::string current_turn_username; // Người chơi có lượt đi tiếp theo
    uint8_t is_game_over;             // 1 nếu ván cờ đã kết thúc, 0 nếu chưa
    std::string message;               // Thông báo (ví dụ: "Chiếu", "Hết giờ", v.v.)

    MessageType getType() const
    {
        return MessageType::GAME_STATUS_UPDATE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        // Thêm FEN
        payload.push_back(static_cast<uint8_t>(fen.size()));
        payload.insert(payload.end(), fen.begin(), fen.end());

        // Thêm current_turn_username
        payload.push_back(static_cast<uint8_t>(current_turn_username.size()));
        payload.insert(payload.end(), current_turn_username.begin(), current_turn_username.end());

        // Thêm cờ is_game_over
        payload.push_back(is_game_over);

        // Thêm thông báo
        payload.push_back(static_cast<uint8_t>(message.size()));
        payload.insert(payload.end(), message.begin(), message.end());
        
        return payload;
    }

    static GameStatusUpdateMessage deserialize(const std::vector<uint8_t> &payload)
    {
        GameStatusUpdateMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.fen = read_string(payload, pos);
        message.current_turn_username = read_string(payload, pos);
        message.is_game_over = read_u8(payload, pos);
        message.message = read_string(payload, pos);
        return message;
    }
};
#pragma endregion GameStatusUpdateMessage

#pragma region GameEndMessage 
// ===== MESSAGE KẾT THÚC TRÒ CHƠI =====
// Được gửi từ server đến cả 2 client để thông báo ván cờ kết thúc
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
    - uint8_t winner_username_length (1 byte): Độ dài tên người thắng
    - char[winner_username_length] winner_username: Tên người thắng (rỗng nếu hòa)
    - uint8_t reason_length (1 byte): Độ dài lý do kết thúc
    - char[reason_length] reason: Lý do kết thúc (chiếu hết, hết giờ, hòa, đầu hàng, v.v.)
    - uint16_t half_moves_count (2 bytes): Số nước đi (tính cả nước của 2 bên)
*/
struct GameEndMessage
{
    std::string game_id;          // ID ván cờ
    std::string winner_username;  // Tên người thắng (rỗng nếu hòa)
    std::string reason;           // Lý do kết thúc ván cờ
    uint16_t half_moves_count;    // Tổng số nước đi trong ván

    MessageType getType() const
    {
        return MessageType::GAME_END;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        // Thêm winner_username
        payload.push_back(static_cast<uint8_t>(winner_username.size()));
        payload.insert(payload.end(), winner_username.begin(), winner_username.end());

        // Thêm lý do
        payload.push_back(static_cast<uint8_t>(reason.size()));
        payload.insert(payload.end(), reason.begin(), reason.end());

        // Thêm số nước đi (2 bytes, Big Endian)
        std::vector<uint8_t> full_moves_count_bytes = to_big_endian_16(half_moves_count);
        payload.insert(payload.end(), full_moves_count_bytes.begin(), full_moves_count_bytes.end());

        return payload;
    }

    static GameEndMessage deserialize(const std::vector<uint8_t> &payload)
    {
        GameEndMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.winner_username = read_string(payload, pos);
        message.reason = read_string(payload, pos);
        message.half_moves_count = read_u16_be(payload, pos);
        return message;
    }
};
#pragma endregion GameEndMessage

#pragma region AutoMatchRequestMessage 
// ===== MESSAGE YÊU CẦU TÌM TRẬN TỰ ĐỘNG =====
// Được gửi từ client đến server để yêu cầu ghép trận tự động
/*
Cấu trúc Payload:
    - uint8_t username_length (1 byte): Độ dài tên người dùng
    - char[username_length] username: Tên người dùng
*/
struct AutoMatchRequestMessage
{
    std::string username;  // Tên người dùng muốn ghép trận

    MessageType getType() const
    {
        return MessageType::AUTO_MATCH_REQUEST;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(username.size()));
        payload.insert(payload.end(), username.begin(), username.end());

        return payload;
    }

    static AutoMatchRequestMessage deserialize(const std::vector<uint8_t> &payload)
    {
        AutoMatchRequestMessage message;
        size_t pos = 0;
        message.username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion AutoMatchRequestMessage

#pragma region AutoMatchFoundMessage 
// ===== MESSAGE TÌM THẤY ĐỐI THỦ =====
// Được gửi từ server đến client để thông báo đã tìm thấy đối thủ phù hợp
/*
Cấu trúc Payload:
    - uint8_t opponent_username_length (1 byte): Độ dài tên đối thủ
    - char[opponent_username_length] opponent_username: Tên đối thủ
    - uint16_t opponent_elo (2 bytes): Điểm Elo của đối thủ
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ sẽ chơi
*/
struct AutoMatchFoundMessage
{
    std::string opponent_username;  // Tên đối thủ được ghép
    uint16_t opponent_elo;         // Điểm Elo của đối thủ
    std::string game_id;           // ID ván cờ sẽ chơi

    MessageType getType() const
    {
        return MessageType::AUTO_MATCH_FOUND;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm tên đối thủ
        payload.push_back(static_cast<uint8_t>(opponent_username.size()));
        payload.insert(payload.end(), opponent_username.begin(), opponent_username.end());

        // Thêm Elo đối thủ (2 bytes, Big Endian)
        std::vector<uint8_t> elo_bytes = to_big_endian_16(opponent_elo);
        payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());

        // Thêm game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static AutoMatchFoundMessage deserialize(const std::vector<uint8_t> &payload)
    {
        AutoMatchFoundMessage message;
        size_t pos = 0;
        message.opponent_username = read_string(payload, pos);
        message.opponent_elo = read_u16_be(payload, pos);
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion AutoMatchFoundMessage

#pragma region AutoMatchAcceptedMessage 
// ===== MESSAGE CHẤP NHẬN TRẬN ĐẤU TỰ ĐỘNG =====
// Được gửi từ client đến server để chấp nhận trận đấu được ghép
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
*/
struct AutoMatchAcceptedMessage
{
    std::string game_id;  // ID ván cờ được chấp nhận

    MessageType getType() const
    {
        return MessageType::AUTO_MATCH_ACCEPTED;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static AutoMatchAcceptedMessage deserialize(const std::vector<uint8_t> &payload)
    {
        AutoMatchAcceptedMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion AutoMatchAcceptedMessage

#pragma region AutoMatchDeclinedMessage 
// ===== MESSAGE TỪ CHỐI TRẬN ĐẤU TỰ ĐỘNG =====
// Được gửi từ client đến server để từ chối trận đấu được ghép
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
*/
struct AutoMatchDeclinedMessage
{
    std::string game_id;  // ID ván cờ bị từ chối

    MessageType getType() const
    {
        return MessageType::AUTO_MATCH_DECLINED;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static AutoMatchDeclinedMessage deserialize(const std::vector<uint8_t> &payload)
    {
        AutoMatchDeclinedMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion AutoMatchDeclinedMessage

#pragma region MatchDeclinedNotificationMessage 
// ===== MESSAGE THÔNG BÁO ĐỐI THỦ TỪ CHỐI =====
// Được gửi từ server đến client để thông báo đối thủ đã từ chối trận đấu
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
*/
struct MatchDeclinedNotificationMessage
{
    std::string game_id;  // ID ván cờ bị từ chối

    MessageType getType() const
    {
        return MessageType::MATCH_DECLINED_NOTIFICATION;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static MatchDeclinedNotificationMessage deserialize(const std::vector<uint8_t> &payload)
    {
        MatchDeclinedNotificationMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion MatchDeclinedNotificationMessage

#pragma region RequestPlayerListMessage 
// ===== MESSAGE YÊU CẦU DANH SÁCH NGƯỜI CHƠI =====
// Được gửi từ client đến server để lấy danh sách người chơi đang online
/*
Cấu trúc Payload:
    - Không có payload
*/
struct RequestPlayerListMessage
{
    MessageType getType() const
    {
        return MessageType::REQUEST_PLAYER_LIST;
    }

    std::vector<uint8_t> serialize() const
    {
        return {}; // Không có dữ liệu payload
    }

    static RequestPlayerListMessage deserialize(const std::vector<uint8_t> &payload)
    {
        // Không có dữ liệu để deserialize
        return RequestPlayerListMessage();
    }
};
#pragma endregion RequestPlayerListMessage

#pragma region PlayerListMessage 
// ===== MESSAGE DANH SÁCH NGƯỜI CHƠI =====
// Được gửi từ server đến client để cung cấp danh sách người chơi
/*
Cấu trúc Payload:
    - uint8_t number_of_players (1 byte): Số lượng người chơi
    - [Player 1][Player 2]... (danh sách người chơi)

Cấu trúc mỗi Player:
    - uint8_t username_length (1 byte): Độ dài tên
    - char[username_length] username: Tên người chơi
    - uint16_t elo (2 bytes): Điểm Elo
    - uint8_t in_game (1 byte): Có đang chơi không (0/1)
    - Nếu in_game = 1:
        - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
        - char[game_id_length] game_id: ID ván cờ đang chơi
*/
struct PlayerListMessage
{
    // Cấu trúc thông tin một người chơi
    struct Player
    {
        std::string username;  // Tên người chơi
        uint16_t elo;         // Điểm Elo
        bool in_game;         // Có đang trong trận không
        std::string game_id;  // ID ván cờ (nếu đang chơi)
    };

    std::vector<Player> players;  // Danh sách người chơi

    MessageType getType() const
    {
        return MessageType::PLAYER_LIST;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;
        // Thêm số lượng người chơi
        payload.push_back(static_cast<uint8_t>(players.size()));

        // Lặp qua từng người chơi
        for (const auto &player : players)
        {
            // Thêm tên người chơi
            payload.push_back(static_cast<uint8_t>(player.username.size()));
            payload.insert(payload.end(), player.username.begin(), player.username.end());

            // Thêm Elo (2 bytes, Big Endian)
            std::vector<uint8_t> elo_bytes = to_big_endian_16(player.elo);
            payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());

            // Thêm cờ in_game
            payload.push_back(static_cast<uint8_t>(player.in_game));
            
            // Nếu đang chơi, thêm game_id
            if (player.in_game) {
                payload.push_back(static_cast<uint8_t>(player.game_id.size()));
                payload.insert(payload.end(), player.game_id.begin(), player.game_id.end());
            }
        }

        return payload;
    }

    static PlayerListMessage deserialize(const std::vector<uint8_t> &payload)
    {
        PlayerListMessage message;
        size_t pos = 0;
        // Đọc số lượng người chơi
        uint8_t number_of_players = read_u8(payload, pos);

        // Lặp qua từng người chơi
        for (uint8_t i = 0; i < number_of_players; ++i)
        {
            Player player;
            player.username = read_string(payload, pos);
            player.elo = read_u16_be(payload, pos);
            player.in_game = static_cast<bool>(read_u8(payload, pos));
            // Nếu đang chơi, đọc game_id
            if (player.in_game) {
                player.game_id = read_string(payload, pos);
            }
            message.players.push_back(player);
        }
        return message;
    }
};
#pragma endregion PlayerListMessage

// ===== CÁC MESSAGE LIÊN QUAN ĐẾN THÁCH ĐẤU =====

#pragma region ChallengeRequestMessage
// ===== MESSAGE YÊU CẦU THÁCH ĐẤU =====
// Được gửi từ client đến server để thách đấu một người chơi cụ thể
struct ChallengeRequestMessage
{
    std::string to_username;  // Tên người được thách đấu

    MessageType getType() const
    {
        return MessageType::CHALLENGE_REQUEST;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(to_username.size()));
        payload.insert(payload.end(), to_username.begin(), to_username.end());

        return payload;
    }

    static ChallengeRequestMessage deserialize(const std::vector<uint8_t> &payload)
    {
        ChallengeRequestMessage message;
        size_t pos = 0;
        message.to_username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion ChallengeRequestMessage

#pragma region ChallengeNotificationMessage
// ===== MESSAGE THÔNG BÁO NHẬN THÁCH ĐẤU =====
// Được gửi từ server đến client để thông báo có người thách đấu
struct ChallengeNotificationMessage
{
    std::string from_username;  // Tên người gửi thách đấu
    uint16_t elo;              // Điểm Elo của người thách đấu

    MessageType getType() const
    {
        return MessageType::CHALLENGE_NOTIFICATION;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm tên người thách đấu
        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        // Thêm Elo (2 bytes, Big Endian)
        std::vector<uint8_t> elo_bytes = to_big_endian_16(elo);
        payload.insert(payload.end(), elo_bytes.begin(), elo_bytes.end());

        return payload;
    }

    static ChallengeNotificationMessage deserialize(const std::vector<uint8_t> &payload)
    {
        ChallengeNotificationMessage message;
        size_t pos = 0;
        message.from_username = read_string(payload, pos);
        message.elo = read_u16_be(payload, pos);
        return message;
    }
};
#pragma endregion ChallengeNotificationMessage

#pragma region ChallengeResponseMessage
// ===== MESSAGE TRẢ LỜI THÁCH ĐẤU =====
// Được gửi từ client đến server để trả lời lời thách đấu (chấp nhận/từ chối)
struct ChallengeResponseMessage
{
    // Enum định nghĩa các loại phản hồi
    enum class Response : uint8_t {
        DECLINED = 0x00,  // Từ chối
        ACCEPTED = 0x01   // Chấp nhận
    };
    Response response;  // Phản hồi của người được thách đấu

    // LƯU Ý: from_username là tên người THÁCH ĐẤU, không phải người được thách
    std::string from_username;

    MessageType getType() const
    {
        return MessageType::CHALLENGE_RESPONSE;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Serialize from_username trước
        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        // Serialize response (1 byte)
        payload.push_back(static_cast<uint8_t>(response));

        return payload;
    }

    static ChallengeResponseMessage deserialize(const std::vector<uint8_t>& payload)
    {
        ChallengeResponseMessage message;
        size_t pos = 0;
        message.from_username = read_string(payload, pos);
        message.response = static_cast<Response>(read_u8(payload, pos));
        return message;
    }
};
#pragma endregion ChallengeResponseMessage

#pragma region ChallengeAcceptedMessage
// ===== MESSAGE CHẤP NHẬN THÁCH ĐẤU =====
// Được gửi từ server đến người thách đấu để thông báo đối thủ đã chấp nhận
struct ChallengeAcceptedMessage
{
    std::string from_username;  // Tên người chấp nhận thách đấu
    std::string game_id;        // ID ván cờ sẽ chơi

    MessageType getType() const
    {
        return MessageType::CHALLENGE_ACCEPTED;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Serialize from_username
        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        // Serialize game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        return payload;
    }

    static ChallengeAcceptedMessage deserialize(const std::vector<uint8_t>& payload)
    {
        ChallengeAcceptedMessage message;
        size_t pos = 0;
        message.from_username = read_string(payload, pos);
        message.game_id = read_string(payload, pos);
        return message;
    }
};
#pragma endregion ChallengeAcceptedMessage

#pragma region ChallengeDeclinedMessage
// ===== MESSAGE TỪ CHỐI THÁCH ĐẤU =====
// Được gửi từ server đến người thách đấu để thông báo đối thủ đã từ chối
struct ChallengeDeclinedMessage
{
    std::string from_username;  // Tên người từ chối thách đấu

    MessageType getType() const
    {
        return MessageType::CHALLENGE_DECLINED;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        return payload;
    }

    static ChallengeDeclinedMessage deserialize(const std::vector<uint8_t> &payload)
    {
        ChallengeDeclinedMessage message;
        size_t pos = 0;
        message.from_username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion ChallengeDeclinedMessage

#pragma region SurrenderMessage
// ===== MESSAGE ĐẦU HÀNG =====
// Được gửi từ client đến server để đầu hàng trong ván đấu
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
*/
struct SurrenderMessage
{
    std::string game_id;        // ID ván cờ
    std::string from_username;  // Tên người đầu hàng

    MessageType getType() const
    {
        return MessageType::SURRENDER;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        // Thêm from_username
        payload.push_back(static_cast<uint8_t>(from_username.size()));
        payload.insert(payload.end(), from_username.begin(), from_username.end());

        return payload;
    }

    static SurrenderMessage deserialize(const std::vector<uint8_t>& payload)
    {
        SurrenderMessage message;
        size_t pos = 0;
        message.game_id = read_string(payload, pos);
        message.from_username = read_string(payload, pos);
        return message;
    }
};
#pragma endregion SurrenderMessage

#pragma region ChallengeErrorMessage
// ===== MESSAGE LỖI THÁCH ĐẤU =====
// Được gửi từ server đến client để thông báo yêu cầu thách đấu không hợp lệ
/*
Cấu trúc Payload:
    - uint8_t error_message_length (1 byte): Độ dài thông báo lỗi
    - char[error_message_length] error_message: Nội dung thông báo lỗi
*/
struct ChallengeErrorMessage
{
    std::string error_message;  // Thông báo lỗi (ví dụ: "Người chơi đang bận")

    MessageType getType() const
    {
        return MessageType::CHALLENGE_ERROR;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        payload.push_back(static_cast<uint8_t>(error_message.size()));
        payload.insert(payload.end(), error_message.begin(), error_message.end());

        return payload;
    }

    static ChallengeErrorMessage deserialize(const std::vector<uint8_t> &payload)
    {
        ChallengeErrorMessage message;

        size_t pos = 0;
        // Đọc độ dài thông báo lỗi
        uint8_t error_message_length = payload[pos++];
        // Tạo chuỗi từ các bytes
        message.error_message = std::string(payload.begin() + pos, payload.begin() + pos + error_message_length);

        return message;
    }
};
#pragma endregion ChallengeErrorMessage

#pragma region GameLogMessage
// ===== MESSAGE NHẬT KÝ VÁN ĐẤU =====
// Được gửi từ server đến cả 2 client sau khi ván cờ kết thúc để cung cấp lịch sử ván đấu
/*
Cấu trúc Payload:
    - uint8_t game_id_length (1 byte): Độ dài ID ván cờ
    - char[game_id_length] game_id: ID ván cờ
    - uint64_t start_time (8 bytes): Thời gian bắt đầu (epoch time tính bằng nanoseconds)
    - uint64_t end_time (8 bytes): Thời gian kết thúc (epoch time tính bằng nanoseconds)
    - uint8_t white_ip_length (1 byte): Độ dài IP quân trắng
    - char[white_ip_length] white_ip: Địa chỉ IP người chơi quân trắng
    - uint8_t black_ip_length (1 byte): Độ dài IP quân đen
    - char[black_ip_length] black_ip: Địa chỉ IP người chơi quân đen
    - uint8_t winner_length (1 byte): Độ dài tên người thắng
    - char[winner_length] winner: Tên người thắng
    - uint8_t reason_length (1 byte): Độ dài lý do kết thúc
    - char[reason_length] reason: Lý do kết thúc
    - uint16_t moves_count (2 bytes): Số lượng nước đi
    - [Move 1][Move 2]... (danh sách các nước đi)
    
Cấu trúc mỗi Move:
    - uint8_t uci_move_length (1 byte): Độ dài nước đi UCI
    - char[uci_move_length] uci_move: Nước đi theo định dạng UCI
*/
struct GameLogMessage
{
    std::string game_id;                 // ID ván cờ
    int64_t start_time;                  // Thời gian bắt đầu
    int64_t end_time;                    // Thời gian kết thúc
    std::string white_ip;                // IP người chơi quân trắng
    std::string black_ip;                // IP người chơi quân đen
    std::string winner;                  // Tên người thắng
    std::string reason;                  // Lý do kết thúc
    std::vector<std::string> moves;      // Danh sách các nước đi

    MessageType getType() const
    {
        return MessageType::GAME_LOG;
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> payload;

        // Thêm game_id
        payload.push_back(static_cast<uint8_t>(game_id.size()));
        payload.insert(payload.end(), game_id.begin(), game_id.end());

        // Thêm start_time (8 bytes, Big Endian)
        // Lặp từ byte cao nhất đến byte thấp nhất
        for (int i = 7; i >= 0; i--)
        {
            // Dịch phải i*8 bit và lấy byte thấp nhất
            payload.push_back(static_cast<uint8_t>((start_time >> (i * 8)) & 0xFF));
        }

        // Thêm end_time (8 bytes, Big Endian)
        for (int i = 7; i >= 0; i--)
        {
            payload.push_back(static_cast<uint8_t>((end_time >> (i * 8)) & 0xFF));
        }

        // Thêm white_ip
        payload.push_back(static_cast<uint8_t>(white_ip.size()));
        payload.insert(payload.end(), white_ip.begin(), white_ip.end());

        // Thêm black_ip
        payload.push_back(static_cast<uint8_t>(black_ip.size()));
        payload.insert(payload.end(), black_ip.begin(), black_ip.end());

        // Thêm winner
        payload.push_back(static_cast<uint8_t>(winner.size()));
        payload.insert(payload.end(), winner.begin(), winner.end());

        // Thêm reason
        payload.push_back(static_cast<uint8_t>(reason.size()));
        payload.insert(payload.end(), reason.begin(), reason.end());

        // Thêm moves_count (2 bytes, Big Endian)
        uint16_t moves_count = static_cast<uint16_t>(moves.size());
        payload.push_back(static_cast<uint8_t>((moves_count >> 8) & 0xFF));  // Byte cao
        payload.push_back(static_cast<uint8_t>(moves_count & 0xFF));         // Byte thấp

        // Thêm từng nước đi
        for (const auto &move : moves)
        {
            payload.push_back(static_cast<uint8_t>(move.size()));
            payload.insert(payload.end(), move.begin(), move.end());
        }

        return payload;
    }

    static GameLogMessage deserialize(const std::vector<uint8_t> &payload)
    {
        GameLogMessage message;

        size_t pos = 0;

        // Đọc game_id
        uint8_t game_id_length = payload[pos++];
        message.game_id = std::string(payload.begin() + pos, payload.begin() + pos + game_id_length);
        pos += game_id_length;

        // Đọc start_time (8 bytes, Big Endian)
        message.start_time = 0;
        for (int i = 0; i < 8; i++)
        {
            // Dịch trái 8 bit và OR với byte hiện tại
            message.start_time = (message.start_time << 8) | payload[pos++];
        }

        // Đọc end_time (8 bytes, Big Endian)
        message.end_time = 0;
        for (int i = 0; i < 8; i++)
        {
            message.end_time = (message.end_time << 8) | payload[pos++];
        }

        // Đọc white_ip
        uint8_t white_ip_length = payload[pos++];
        message.white_ip = std::string(payload.begin() + pos, payload.begin() + pos + white_ip_length);
        pos += white_ip_length;

        // Đọc black_ip
        uint8_t black_ip_length = payload[pos++];
        message.black_ip = std::string(payload.begin() + pos, payload.begin() + pos + black_ip_length);
        pos += black_ip_length;

        // Đọc winner
        uint8_t winner_length = payload[pos++];
        message.winner = std::string(payload.begin() + pos, payload.begin() + pos + winner_length);
        pos += winner_length;

        // Đọc reason
        uint8_t reason_length = payload[pos++];
        message.reason = std::string(payload.begin() + pos, payload.begin() + pos + reason_length);
        pos += reason_length;

        // Đọc moves_count (2 bytes, Big Endian)
        // Byte cao << 8 | Byte thấp
        uint16_t moves_count = (static_cast<uint16_t>(payload[pos]) << 8) | static_cast<uint16_t>(payload[pos + 1]);
        pos += 2;

        // Đọc từng nước đi
        for (uint16_t i = 0; i < moves_count; i++)
        {
            uint8_t move_length = payload[pos++];
            message.moves.push_back(std::string(payload.begin() + pos, payload.begin() + pos + move_length));
            pos += move_length;
        }

        return message;
    }
};
#pragma endregion GameLogMessage

#endif // MESSAGE_HPP
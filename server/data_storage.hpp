#ifndef DATA_STORAGE_HPP
#define DATA_STORAGE_HPP

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <unistd.h>
#include <limits.h>

#include "../libraries/json.hpp"
#include "../common/json_handler.hpp"
#include "../common/const.hpp"

// Sử dụng thư viện nlohmann/json để xử lý dữ liệu định dạng JSON
using json = nlohmann::json; // alias

/**
 * @brief Cấu trúc dữ liệu đại diện cho một người dùng (User).
 */
struct UserModel
{
    std::string username; // Tên đăng nhập của người dùng
    uint16_t elo;        // Điểm số (ELO) của người dùng

    /**
     * @brief Chuyển đổi đối tượng UserModel sang định dạng JSON để lưu trữ.
     * @return Đối tượng json chứa thông tin elo.
     */
    json serialize() const
    {
        return {
            {"elo", elo}};
    }

    /**
     * @brief Chuyển đổi từ dữ liệu JSON và username thành đối tượng UserModel.
     * @param username Tên người dùng.
     * @param j Đối tượng json chứa dữ liệu elo.
     * @return Đối tượng UserModel hoàn chỉnh.
     */
    static UserModel deserialize(const std::string &username, const json &j)
    {
        return UserModel{
            username,
            j.at("elo").get<uint16_t>()
        };
    }
};

/**
 * @brief Cấu trúc dữ liệu đại diện cho một trận đấu (Match).
 */
struct MatchModel
{
    std::string game_id;        // ID duy nhất của trận đấu
    std::string white_username; // Tên người chơi quân Trắng
    std::string black_username; // Tên người chơi quân Đen
    std::string white_ip;       // Địa chỉ IP của người chơi quân Trắng
    std::string black_ip;       // Địa chỉ IP của người chơi quân Đen
    std::string start_fen;      // Trạng thái bàn cờ lúc bắt đầu (định dạng FEN)
    
    // Thời điểm bắt đầu và kết thúc trận đấu
    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::chrono::time_point<std::chrono::system_clock> end_time;

    /**
     * @brief Cấu trúc đại diện cho một nước đi trong trận đấu.
     */
    struct Move
    {
        std::string uci_move; // Nước đi theo chuẩn UCI (ví dụ: e2e4)
        std::string fen;      // Trạng thái bàn cờ sau nước đi này
        std::chrono::time_point<std::chrono::system_clock> move_time; // Thời điểm thực hiện nước đi
    };

    std::vector<Move> moves; // Danh sách các nước đi đã thực hiện
    std::string result;      // Kết quả trận đấu (ví dụ: "1-0", "0-1", "1/2-1/2")
    std::string reason;      // Lý do kết thúc (ví dụ: "checkmate", "timeout", "resign")

    /**
     * @brief Chuyển đổi thông tin trận đấu sang định dạng JSON.
     */
    json serialize() const
    {
        json j;
        j["white_username"] = white_username;
        j["black_username"] = black_username;
        j["white_ip"] = white_ip;
        j["black_ip"] = black_ip;
        j["start_fen"] = start_fen;
        // Chuyển đổi thời gian sang dạng số (nanoseconds) để lưu vào JSON
        j["start_time"] = start_time.time_since_epoch().count();
        j["end_time"] = end_time.time_since_epoch().count();

        json moves_json;
        for (const auto &move : moves)
        {
            json move_json;
            move_json["uci_move"] = move.uci_move;
            move_json["fen"] = move.fen;
            move_json["move_time"] = move.move_time.time_since_epoch().count();
            moves_json.push_back(move_json);
        }
        j["moves"] = moves_json;

        j["result"] = result;
        j["reason"] = reason;

        return j;
    }

    /**
     * @brief Khôi phục thông tin trận đấu từ dữ liệu JSON.
     */
    static MatchModel deserialize(const std::string &game_id, const json &j)
    {
        MatchModel game;
        game.game_id = game_id;
        game.white_username = j.at("white_username").get<std::string>();
        game.black_username = j.at("black_username").get<std::string>();
        game.white_ip = j.value("white_ip", "");
        game.black_ip = j.value("black_ip", "");
        game.start_fen = j.at("start_fen").get<std::string>();
        
        // Khôi phục thời gian từ số nanoseconds
        game.start_time = std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(j.at("start_time").get<int64_t>()));
        game.end_time = std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(j.value("end_time", (int64_t)0)));

        for (const auto &move_json : j.at("moves"))
        {
            MatchModel::Move move;
            move.uci_move = move_json.at("uci_move").get<std::string>();
            move.fen = move_json.at("fen").get<std::string>();
            move.move_time = std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(move_json.at("move_time").get<int64_t>()));
            game.moves.push_back(move);
        }

        game.result = j.at("result").get<std::string>();
        game.reason = j.at("reason").get<std::string>();

        return game;
    }
};

/**
 * @brief Lớp DataStorage quản lý việc lưu trữ và truy xuất dữ liệu (Người dùng và Trận đấu).
 * 
 * Sử dụng mẫu thiết kế Singleton: Đảm bảo chỉ có một đối tượng duy nhất tồn tại trong suốt chương trình.
 * Sử dụng Mutex để đảm bảo an toàn khi nhiều luồng (thread) cùng truy cập dữ liệu (Thread-safe).
 */
class DataStorage
{
public:
    /**
     * @brief Lấy instance duy nhất của lớp DataStorage.
     */
    static DataStorage &getInstance()
    {
        static DataStorage instance;
        return instance;
    }

    /**
     * @brief Đăng ký một người dùng mới.
     *
     * @param username Tên người dùng.
     * @param elo Điểm Elo khởi tạo.
     * @return true nếu thành công, false nếu username đã tồn tại.
     */
    bool registerUser(const std::string &username, const uint16_t elo = Const::DEFAULT_ELO)
    {
        // Khóa mutex để đảm bảo không có luồng nào khác can thiệp khi đang ghi dữ liệu
        std::lock_guard<std::mutex> lock(users_mutex);

        if (users.find(username) != users.end())
        {
            return false; // Username đã tồn tại
        }

        users[username] = UserModel{
            username,
            elo
        };

        saveUsersData(); // Lưu lại vào file JSON ngay lập tức

        return true;
    }

    /**
     * @brief Kiểm tra xem người dùng có tồn tại trong hệ thống hay không.
     */
    bool validateUser(const std::string &username)
    {
        std::lock_guard<std::mutex> lock(users_mutex);
        return users.find(username) != users.end();
    }

    /**
     * @brief Lấy điểm ELO của một người dùng.
     */
    uint16_t getUserELO(const std::string &username)
    {
        std::lock_guard<std::mutex> lock(users_mutex);

        auto it = users.find(username);
        if (it != users.end())
        {
            return it->second.elo;
        }
        return 0; // Trả về 0 nếu không tìm thấy
    }

    /**
     * @brief Cập nhật điểm ELO cho người dùng.
     */
    bool updateUserELO(const std::string &username, const uint16_t elo)
    {
        std::lock_guard<std::mutex> lock(users_mutex);

        auto it = users.find(username);
        if (it != users.end())
        {
            it->second.elo = elo;
            saveUsersData(); // Lưu thay đổi vào file
            return true;
        }
        return false;
    }

    /**
     * @brief Lấy danh sách toàn bộ người dùng.
     */
    std::unordered_map<std::string, UserModel> getPlayerList()
    {
        std::lock_guard<std::mutex> lock(users_mutex);
        return users;
    }

    /**
     * @brief Tính thứ hạng (Rank) của người dùng dựa trên điểm ELO.
     * 
     * @param username Tên người dùng.
     * @return Thứ hạng (1 là cao nhất), 0 nếu không tìm thấy.
     */
    int getUserRank(const std::string &username)
    {
        std::lock_guard<std::mutex> lock(users_mutex);
        
        auto it = users.find(username);
        if (it == users.end())
        {
            return 0;
        }
        
        uint16_t user_elo = it->second.elo;
        int rank = 1;
        
        // Duyệt qua tất cả người dùng để đếm xem có bao nhiêu người điểm cao hơn
        for (const auto &[name, user] : users)
        {
            if (user.elo > user_elo)
            {
                rank++;
            }
        }
        
        return rank;
    }

public:
    /**
     * @brief Đăng ký một trận đấu mới vào hệ thống.
     *
     * @param game_id ID trận đấu.
     * @param white_username Người chơi trắng.
     * @param black_username Người chơi đen.
     * @param start_fen Thế cờ khởi đầu.
     * @param white_ip IP người chơi trắng.
     * @param black_ip IP người chơi đen.
     * @return true nếu thành công.
     */
    bool registerMatch(const std::string &game_id, const std::string &white_username, const std::string &black_username, const std::string &start_fen, const std::string &white_ip = "", const std::string &black_ip = "")
    {
        std::lock_guard<std::mutex> lock(matches_mutex);

        if (matches.find(game_id) != matches.end())
        {
            return false; // Trận đấu đã tồn tại
        }

        MatchModel match;
        match.game_id = game_id;
        match.white_username = white_username;
        match.black_username = black_username;
        match.white_ip = white_ip;
        match.black_ip = black_ip;
        match.start_fen = start_fen;
        match.start_time = std::chrono::system_clock::now(); // Ghi nhận thời gian hiện tại
        match.end_time = std::chrono::time_point<std::chrono::system_clock>();  // Chưa kết thúc
        match.result = "";
        match.reason = "";
        
        matches[game_id] = match;

        saveMatchesData(); // Lưu vào file matches.json
        return true;
    }

    /**
     * @brief Cập nhật kết quả cuối cùng của trận đấu.
     */
    bool updateMatchResult(const std::string &game_id, const std::string &result, const std::string &reason)
    {
        std::lock_guard<std::mutex> lock(matches_mutex);

        auto it = matches.find(game_id);
        if (it != matches.end())
        {
            it->second.result = result;
            it->second.reason = reason;
            it->second.end_time = std::chrono::system_clock::now(); // Ghi nhận thời gian kết thúc
            saveMatchesData();
            return true;
        }
        return false;
    }

    /**
     * @brief Lấy thông tin chi tiết của một trận đấu qua ID.
     */
    MatchModel getMatch(const std::string &game_id)
    {
        std::lock_guard<std::mutex> lock(matches_mutex);

        auto it = matches.find(game_id);
        if (it != matches.end())
        {
            return it->second;
        }
        throw std::runtime_error("Match not found.");
    }

    /**
     * @brief Lưu lại một nước đi mới vào lịch sử trận đấu.
     */
    bool addMove(const std::string &game_id, const std::string &uci_move, const std::string &fen)
    {
        std::lock_guard<std::mutex> lock(matches_mutex);

        auto it = matches.find(game_id);
        if (it != matches.end())
        {
            MatchModel::Move move;
            move.uci_move = uci_move;
            move.fen = fen;
            move.move_time = std::chrono::system_clock::now();
            it->second.moves.push_back(move);
            saveMatchesData();
            return true;
        }
        return false;
    }


private:
    // Dữ liệu người dùng: ánh xạ từ username sang UserModel
    std::unordered_map<std::string, UserModel> users; 
    std::mutex users_mutex; // Mutex bảo vệ dữ liệu người dùng

    // Dữ liệu trận đấu: ánh xạ từ game_id sang MatchModel
    std::unordered_map<std::string, MatchModel> matches; 
    std::mutex matches_mutex; // Mutex bảo vệ dữ liệu trận đấu

    // Các phương thức private để ngăn chặn việc tạo thêm instance (Singleton)
    ~DataStorage() = default;
    DataStorage(const DataStorage &) = delete;
    DataStorage &operator=(const DataStorage &) = delete;

    /**
     * @brief Xác định đường dẫn thư mục chứa dữ liệu (data/) tương đối với file thực thi.
     */
    std::string getDataPath()
    {
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        std::string exePath = "";
        if (count != -1)
        {
            exePath = std::string(result, count);
            size_t lastSlash = exePath.find_last_of("/\\");
            exePath = exePath.substr(0, lastSlash);
        }

        // Dữ liệu được lưu trong thư mục ../data/ so với vị trí file chạy
        return exePath + "/../data/";
    }

    /**
     * @brief Constructor: Tự động tải dữ liệu từ các file JSON khi khởi tạo.
     */
    DataStorage()
    {
        std::string dataPath = getDataPath();

        // Tải dữ liệu người dùng
        json users_j = JSONHandler::readJSON(dataPath + "users.json");
        for (auto it = users_j.begin(); it != users_j.end(); ++it)
        {
            std::string username = it.key();
            users[username] = UserModel::deserialize(username, it.value());
        }

        // Tải dữ liệu các trận đấu
        json matches_j = JSONHandler::readJSON(dataPath + "matches.json");
        for (auto it = matches_j.begin(); it != matches_j.end(); ++it)
        {
            std::string game_id = it.key();
            matches[game_id] = MatchModel::deserialize(game_id, it.value());
        }
    }

    /**
     * @brief Ghi toàn bộ dữ liệu người dùng hiện tại vào file users.json.
     */
    bool saveUsersData()
    {
        json j;
        for (const auto &[username, user] : users)
        {
            j[username] = user.serialize();
        }
        std::string dataPath = getDataPath();
        JSONHandler::writeJSON(dataPath + "users.json", j);
        return true;
    }

    /**
     * @brief Ghi toàn bộ dữ liệu trận đấu hiện tại vào file matches.json.
     */
    bool saveMatchesData()
    {
        json j;
        for (const auto &[game_id, match] : matches)
        {
            j[game_id] = match.serialize();
        }
        std::string dataPath = getDataPath();
        JSONHandler::writeJSON(dataPath + "matches.json", j);
        return true;
    }
};

#endif // DATA_STORAGE_HPP
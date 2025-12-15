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

using json = nlohmann::json;

struct UserModel
{
    std::string username;
    uint16_t elo;
    std::vector<std::string> match_history;

    json serialize() const
    {
        return {
            {"elo", elo},
            {"match_history", match_history}};
    }

    static UserModel deserialize(const std::string &username, const json &j)
    {
        return UserModel{
            username,
            j.at("elo").get<uint16_t>(),
            j.value("match_history", std::vector<std::string>{}) // Sử dụng giá trị mặc định
        };
    }
};

struct MatchModel
{
    std::string game_id;
    std::string white_username;
    std::string black_username;
    std::string start_fen;
    std::chrono::time_point<std::chrono::system_clock> start_time;

    struct Move
    {
        std::string uci_move;
        std::string fen;
        std::chrono::time_point<std::chrono::system_clock> move_time;
    };

    std::vector<Move> moves;
    std::string result;
    std::string reason;

    json serialize() const
    {
        json j;
        j["white_username"] = white_username;
        j["black_username"] = black_username;
        j["start_fen"] = start_fen;
        j["start_time"] = start_time.time_since_epoch().count();

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

    static MatchModel deserialize(const std::string &game_id, const json &j)
    {
        MatchModel game;
        game.game_id = game_id;
        game.white_username = j.at("white_username").get<std::string>();
        game.black_username = j.at("black_username").get<std::string>();
        game.start_fen = j.at("start_fen").get<std::string>();
        game.start_time = std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(j.at("start_time").get<int64_t>()));

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
 * @brief Lớp DataStorage là một Singleton quản lý dữ liệu người dùng trong ứng dụng TCP_Chess.
 *
 * Các chức năng chính:
 *
 * - Đăng ký người dùng mới với tên và điểm ELO mặc định.
 *
 * - Xác thực sự tồn tại của người dùng.
 *
 * - Lấy và cập nhật điểm ELO của người dùng.
 *
 * @note Lớp này không thể sao chép hoặc gán để đảm bảo chỉ có một instance được tồn tại.
 */
class DataStorage
{
public:
    static DataStorage &getInstance()
    {
        static DataStorage instance;
        return instance;
    }

    /**
     * Đăng ký một người dùng mới.
     *
     * @param username Tên người dùng cần đăng ký.
     * @param elo Điểm Elo khởi tạo (mặc định: Const::DEFAULT_ELO).
     * @return true nếu đăng ký thành công, false nếu tên người dùng đã tồn tại.
     */
    bool registerUser(const std::string &username, const uint16_t elo = Const::DEFAULT_ELO)
    {
        std::lock_guard<std::mutex> lock(users_mutex);

        if (users.find(username) != users.end())
        {
            return false; // Username đã tồn tại
        }

        users[username] = UserModel{
            username,
            elo,
            {} // match_history ban đầu là rỗng;
        };

        saveUsersData();

        return true;
    }

    /**
     * Kiểm tra tính hợp lệ của người dùng.
     *
     * @param username Tên người dùng cần xác thực.
     * @return Trả về true nếu người dùng tồn tại, ngược lại trả về false.
     */
    bool validateUser(const std::string &username)
    {
        std::lock_guard<std::mutex> lock(users_mutex);

        return users.find(username) != users.end();
    }

    uint16_t getUserELO(const std::string &username)
    {
        std::lock_guard<std::mutex> lock(users_mutex);

        auto it = users.find(username);
        if (it != users.end())
        {
            return it->second.elo;
        }
        return 0; // ELO mặc định nếu không tìm thấy
    }

    bool updateUserELO(const std::string &username, const uint16_t elo)
    {
        std::lock_guard<std::mutex> lock(users_mutex);

        auto it = users.find(username);
        if (it != users.end())
        {
            it->second.elo = elo;
            saveUsersData();
            return true;
        }
        return false;
    }

    bool addMatchToUserHistory(const std::string &username, const std::string &game_id)
    {
        std::lock_guard<std::mutex> lock(users_mutex);

        auto it = users.find(username);
        if (it != users.end())
        {
            it->second.match_history.push_back(game_id);
            saveUsersData();
            return true;
        }
        return false;
    }

    std::unordered_map<std::string, UserModel> getPlayerList()
    {
        std::lock_guard<std::mutex> lock(users_mutex);
        return users;
    }

public:
    /**
     * @brief Đăng ký một trận đấu mới.
     *
     * @param game_id ID của trận đấu.
     * @param white_username Tên người chơi cầm quân trắng.
     * @param black_username Tên người chơi cầm quân đen.
     * @param start_fen Vị trí FEN khởi đầu.
     * @return true nếu đăng ký thành công, false nếu trận đấu đã tồn tại.
     */
    bool registerMatch(const std::string &game_id, const std::string &white_username, const std::string &black_username, const std::string &start_fen)
    {
        std::lock_guard<std::mutex> lock(matches_mutex);

        if (matches.find(game_id) != matches.end())
        {
            return false; // Trận đấu đã tồn tại
        }

        matches[game_id] = MatchModel{
            game_id,
            white_username,
            black_username,
            start_fen,
            std::chrono::system_clock::now(),
            {}, // moves ban đầu là rỗng
            "", // result ban đầu
            ""  // reason ban đầu
        };

        saveMatchesData();
        return true;
    }

    /**
     * @brief Cập nhật kết quả của một trận đấu.
     *
     * @param game_id ID của trận đấu.
     * @param result Kết quả trận đấu.
     * @param reason Lý do kết quả.
     * @return true nếu cập nhật thành công, false nếu không tìm thấy trận đấu.
     */
    bool updateMatchResult(const std::string &game_id, const std::string &result, const std::string &reason)
    {
        std::lock_guard<std::mutex> lock(matches_mutex);

        auto it = matches.find(game_id);
        if (it != matches.end())
        {
            it->second.result = result;
            it->second.reason = reason;
            saveMatchesData();
            return true;
        }
        return false;
    }

    /**
     * @brief Lấy thông tin một trận đấu.
     *
     * @param game_id ID của trận đấu.
     * @return MatchModel nếu tìm thấy, hoặc ném ngoại lệ nếu không tìm thấy.
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
     * @brief Thêm một nước đi vào trận đấu.
     *
     * @param game_id ID của trận đấu.
     * @param move Nước đi cần thêm.
     * @return true nếu thành công, false nếu không tìm thấy trận đấu.
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

    /**
     * @brief Lấy lịch sử trận đấu của một người chơi.
     *
     * @param username Tên người chơi cần lấy lịch sử.
     * @return std::vector<MatchModel> chứa lịch sử trận đấu của người chơi.
     */
    std::vector<MatchModel> getMatchHistory(const std::string &username)
    {
        std::lock_guard<std::mutex> lock(matches_mutex);

        std::vector<MatchModel> match_history;
        for (const auto &[game_id, match] : matches)
        {
            if (match.white_username == username || match.black_username == username)
            {
                match_history.push_back(match);
            }
        }
        return match_history;
    }

private:
    std::unordered_map<std::string, UserModel> users; // mapping username -> User
    std::mutex users_mutex;

    std::unordered_map<std::string, MatchModel> matches; // mapping game_id -> Match
    std::mutex matches_mutex;

    ~DataStorage() = default;
    DataStorage(const DataStorage &) = delete;
    DataStorage &operator=(const DataStorage &) = delete;

    std::string getDataPath()
    {
        // Get the path of the executable
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        std::string exePath = "";
        if (count != -1)
        {
            exePath = std::string(result, count);
            size_t lastSlash = exePath.find_last_of("/\\");
            exePath = exePath.substr(0, lastSlash);
        }

        // Construct the data path relative to the executable
        return exePath + "/../data/";
    }

    DataStorage()
    {
        std::string dataPath = getDataPath();

        // Load users.json
        json users_j = JSONHandler::readJSON(dataPath + "users.json");
        for (auto it = users_j.begin(); it != users_j.end(); ++it)
        {
            std::string username = it.key();
            users[username] = UserModel::deserialize(username, it.value());
        }

        // Load matches.json
        json matches_j = JSONHandler::readJSON(dataPath + "matches.json");
        for (auto it = matches_j.begin(); it != matches_j.end(); ++it)
        {
            std::string game_id = it.key();
            matches[game_id] = MatchModel::deserialize(game_id, it.value());
        }
    }

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
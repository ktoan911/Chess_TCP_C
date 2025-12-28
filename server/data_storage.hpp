#ifndef DATA_STORAGE_HPP
#define DATA_STORAGE_HPP

#include <chrono>
#include <limits.h>
#include <mutex>
#include <string>
#include <unistd.h>
#include <unordered_map>

#include "../common/const.hpp"
#include "../common/json_handler.hpp"
#include "../libraries/json.hpp"
#include "structs.hpp"

/**
 * @brief Lớp DataStorage quản lý việc lưu trữ và truy xuất dữ liệu (Người dùng
 * và Trận đấu).
 *
 * Sử dụng mẫu thiết kế Singleton: Đảm bảo chỉ có một đối tượng duy nhất tồn tại
 * trong suốt chương trình. Sử dụng Mutex để đảm bảo an toàn khi nhiều luồng
 * (thread) cùng truy cập dữ liệu (Thread-safe).
 */
class DataStorage {
public:
  /**
   * @brief Lấy instance duy nhất của lớp DataStorage.
   */
  static DataStorage &getInstance() {
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
  bool registerUser(const std::string &username,
                    const uint16_t elo = Const::DEFAULT_ELO) {
    // Khóa mutex để đảm bảo không có luồng nào khác can thiệp khi đang ghi dữ
    // liệu
    std::lock_guard<std::mutex> lock(users_mutex);

    if (users.find(username) != users.end()) {
      return false; // Username đã tồn tại
    }

    users[username] = UserModel{username, elo};

    saveUsersData(); // Lưu lại vào file JSON ngay lập tức

    return true;
  }

  /**
   * @brief Kiểm tra xem người dùng có tồn tại trong hệ thống hay không.
   */
  bool validateUser(const std::string &username) {
    std::lock_guard<std::mutex> lock(users_mutex);
    return users.find(username) != users.end();
  }

  /**
   * @brief Lấy điểm ELO của một người dùng.
   */
  uint16_t getUserELO(const std::string &username) {
    std::lock_guard<std::mutex> lock(users_mutex);

    auto it = users.find(username);
    if (it != users.end()) {
      return it->second.elo;
    }
    return 0; // Trả về 0 nếu không tìm thấy
  }

  /**
   * @brief Cập nhật điểm ELO cho người dùng.
   */
  bool updateUserELO(const std::string &username, const uint16_t elo) {
    std::lock_guard<std::mutex> lock(users_mutex);

    auto it = users.find(username);
    if (it != users.end()) {
      it->second.elo = elo;
      saveUsersData(); // Lưu thay đổi vào file
      return true;
    }
    return false;
  }

  /**
   * @brief Lấy danh sách toàn bộ người dùng.
   */
  std::unordered_map<std::string, UserModel> getPlayerList() {
    std::lock_guard<std::mutex> lock(users_mutex);
    return users;
  }

  /**
   * @brief Tính thứ hạng (Rank) của người dùng dựa trên điểm ELO.
   *
   * @param username Tên người dùng.
   * @return Thứ hạng (1 là cao nhất), 0 nếu không tìm thấy.
   */
  int getUserRank(const std::string &username) {
    std::lock_guard<std::mutex> lock(users_mutex);

    auto it = users.find(username);
    if (it == users.end()) {
      return 0;
    }

    uint16_t user_elo = it->second.elo;
    int rank = 1;

    // Duyệt qua tất cả người dùng để đếm xem có bao nhiêu người điểm cao hơn
    for (const auto &[name, user] : users) {
      if (user.elo > user_elo) {
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
  bool registerMatch(const std::string &game_id,
                     const std::string &white_username,
                     const std::string &black_username,
                     const std::string &start_fen,
                     const std::string &white_ip = "",
                     const std::string &black_ip = "") {
    std::lock_guard<std::mutex> lock(matches_mutex);

    if (matches.find(game_id) != matches.end()) {
      return false; // Trận đấu đã tồn tại
    }

    MatchModel match;
    match.game_id = game_id;
    match.white_username = white_username;
    match.black_username = black_username;
    match.white_ip = white_ip;
    match.black_ip = black_ip;
    match.start_fen = start_fen;
    match.start_time =
        std::chrono::system_clock::now(); // Ghi nhận thời gian hiện tại
    match.end_time =
        std::chrono::time_point<std::chrono::system_clock>(); // Chưa kết thúc
    match.result = "";
    match.reason = "";

    matches[game_id] = match;

    saveMatchesData(); // Lưu vào file matches.json
    return true;
  }

  /**
   * @brief Cập nhật kết quả cuối cùng của trận đấu.
   */
  bool updateMatchResult(const std::string &game_id, const std::string &result,
                         const std::string &reason) {
    std::lock_guard<std::mutex> lock(matches_mutex);

    auto it = matches.find(game_id);
    if (it != matches.end()) {
      it->second.result = result;
      it->second.reason = reason;
      it->second.end_time =
          std::chrono::system_clock::now(); // Ghi nhận thời gian kết thúc
      saveMatchesData();
      return true;
    }
    return false;
  }

  /**
   * @brief Lấy thông tin chi tiết của một trận đấu qua ID.
   */
  MatchModel getMatch(const std::string &game_id) {
    std::lock_guard<std::mutex> lock(matches_mutex);

    auto it = matches.find(game_id);
    if (it != matches.end()) {
      return it->second;
    }
    throw std::runtime_error("Match not found.");
  }

  /**
   * @brief Lưu lại một nước đi mới vào lịch sử trận đấu.
   */
  bool addMove(const std::string &game_id, const std::string &uci_move,
               const std::string &fen) {
    std::lock_guard<std::mutex> lock(matches_mutex);

    auto it = matches.find(game_id);
    if (it != matches.end()) {
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
   * @brief Xác định đường dẫn thư mục chứa dữ liệu (data/) tương đối với file
   * thực thi.
   */
  std::string getDataPath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    std::string exePath = "";
    if (count != -1) {
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
  DataStorage() {
    std::string dataPath = getDataPath();

    // Tải dữ liệu người dùng
    json users_j = JSONHandler::readJSON(dataPath + "users.json");
    for (auto it = users_j.begin(); it != users_j.end(); ++it) {
      std::string username = it.key();
      users[username] = UserModel::deserialize(username, it.value());
    }

    // Tải dữ liệu các trận đấu
    json matches_j = JSONHandler::readJSON(dataPath + "matches.json");
    for (auto it = matches_j.begin(); it != matches_j.end(); ++it) {
      std::string game_id = it.key();
      matches[game_id] = MatchModel::deserialize(game_id, it.value());
    }
  }

  /**
   * @brief Ghi toàn bộ dữ liệu người dùng hiện tại vào file users.json.
   */
  bool saveUsersData() {
    json j;
    for (const auto &[username, user] : users) {
      j[username] = user.serialize();
    }
    std::string dataPath = getDataPath();
    JSONHandler::writeJSON(dataPath + "users.json", j);
    return true;
  }

  /**
   * @brief Ghi toàn bộ dữ liệu trận đấu hiện tại vào file matches.json.
   */
  bool saveMatchesData() {
    json j;
    for (const auto &[game_id, match] : matches) {
      j[game_id] = match.serialize();
    }
    std::string dataPath = getDataPath();
    JSONHandler::writeJSON(dataPath + "matches.json", j);
    return true;
  }
};

#endif // DATA_STORAGE_HPP
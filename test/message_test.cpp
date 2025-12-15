// test.cpp
#include <iostream>
#include "../common/message.hpp"

void test_register_message() {
    // Arrange
    RegisterMessage original_message;
    original_message.username = "testuser";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    RegisterMessage deserialized_message = RegisterMessage::deserialize(serialized);

    // Assert
    std::cout << "RegisterMessage Test: " << (original_message.username == deserialized_message.username ? "Passed" : "Failed") << std::endl;
}

void test_register_failure_message() {
    // Arrange
    RegisterFailureMessage original_message;
    original_message.error_message = "error occurred";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    RegisterFailureMessage deserialized_message = RegisterFailureMessage::deserialize(serialized);

    // Assert
    std::cout << "RegisterFailureMessage Test: " << (original_message.error_message == deserialized_message.error_message ? "Passed" : "Failed") << std::endl;
}

void test_login_message() {
    // Arrange
    LoginMessage original_message;
    original_message.username = "testuser";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    LoginMessage deserialized_message = LoginMessage::deserialize(serialized);

    // Assert
    std::cout << "LoginMessage Test: " << (original_message.username == deserialized_message.username ? "Passed" : "Failed") << std::endl;
}

void test_login_success_message() {
    // Arrange
    LoginSuccessMessage original_message;
    original_message.username = "testuser";
    original_message.elo = 1234;

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    LoginSuccessMessage deserialized_message = LoginSuccessMessage::deserialize(serialized);

    // Assert
    bool username_match = original_message.username == deserialized_message.username;
    bool elo_match = original_message.elo == deserialized_message.elo;

    std::cout << "LoginSuccessMessage Test: " << (username_match && elo_match ? "Passed" : "Failed") << std::endl;
}

void test_login_failure_message() {
    // Arrange
    LoginFailureMessage original_message;
    original_message.error_message = "login failed";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    LoginFailureMessage deserialized_message = LoginFailureMessage::deserialize(serialized);

    // Assert
    std::cout << "LoginFailureMessage Test: " << (original_message.error_message == deserialized_message.error_message ? "Passed" : "Failed") << std::endl;
}

void test_game_start_message() {
    // Arrange
    GameStartMessage original_message;
    original_message.game_id = "game123";
    original_message.player1_username = "player1";
    original_message.player2_username = "player2";
    original_message.starting_player_username = "player1";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    GameStartMessage deserialized_message = GameStartMessage::deserialize(serialized);

    // Assert
    bool game_id_match = original_message.game_id == deserialized_message.game_id;
    bool player1_match = original_message.player1_username == deserialized_message.player1_username;
    bool player2_match = original_message.player2_username == deserialized_message.player2_username;
    bool starting_player_match = original_message.starting_player_username == deserialized_message.starting_player_username;

    std::cout << "GameStartMessage Test: " << (game_id_match && player1_match && player2_match && starting_player_match ? "Passed" : "Failed") << std::endl;
}

void test_move_message() {
    // Arrange
    MoveMessage original_message;
    original_message.game_id = "game123";
    original_message.uci_move = "e2e4";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    MoveMessage deserialized_message = MoveMessage::deserialize(serialized);

    // Assert
    bool game_id_match = original_message.game_id == deserialized_message.game_id;
    bool uci_move_match = original_message.uci_move == deserialized_message.uci_move;

    std::cout << "MoveMessage Test: " << (game_id_match && uci_move_match ? "Passed" : "Failed") << std::endl;
}

void test_game_status_update_message() {
    // Arrange
    GameStatusUpdateMessage original_message;
    original_message.game_id = "game123";
    original_message.fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    original_message.current_turn_username = "player1";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    GameStatusUpdateMessage deserialized_message = GameStatusUpdateMessage::deserialize(serialized);

    // Assert
    bool game_id_match = original_message.game_id == deserialized_message.game_id;
    bool fen_match = original_message.fen == deserialized_message.fen;
    bool current_turn_match = original_message.current_turn_username == deserialized_message.current_turn_username;

    std::cout << "GameStatusUpdateMessage Test: " << (game_id_match && fen_match && current_turn_match ? "Passed" : "Failed") << std::endl;
}

// void test_game_end_message() {
//     // Arrange
//     GameEndMessage original_message;
//     original_message.game_id = "game123";
//     original_message.winner_username = "player1";
//     original_message.result = 1;
//     original_message.reason = "checkmate";
//     original_message.half_moves_count = 40;

//     // Act
//     std::vector<uint8_t> serialized = original_message.serialize();
//     GameEndMessage deserialized_message = GameEndMessage::deserialize(serialized);

//     // Assert
//     bool game_id_match = original_message.game_id == deserialized_message.game_id;
//     bool winner_match = original_message.winner_username == deserialized_message.winner_username;
//     bool result_match = original_message.result == deserialized_message.result;
//     bool reason_match = original_message.reason == deserialized_message.reason;
//     bool moves_count_match = original_message.half_moves_count == deserialized_message.half_moves_count;

//     std::cout << "GameEndMessage Test: " << (game_id_match && winner_match && result_match && reason_match && moves_count_match ? "Passed" : "Failed") << std::endl;
// }

void test_game_status_update_message_extended() {
    // Arrange
    GameStatusUpdateMessage original_message;
    original_message.game_id = "game123";
    original_message.fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    original_message.current_turn_username = "player1";
    original_message.is_game_over = 0;
    original_message.message = "Game is ongoing";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    GameStatusUpdateMessage deserialized_message = GameStatusUpdateMessage::deserialize(serialized);

    // Assert
    bool game_id_match = original_message.game_id == deserialized_message.game_id;
    bool fen_match = original_message.fen == deserialized_message.fen;
    bool current_turn_match = original_message.current_turn_username == deserialized_message.current_turn_username;
    bool is_game_over_match = original_message.is_game_over == deserialized_message.is_game_over;
    bool message_match = original_message.message == deserialized_message.message;

    std::cout << "GameStatusUpdateMessage Extended Test: " 
              << (game_id_match && fen_match && current_turn_match && is_game_over_match && message_match ? "Passed" : "Failed") 
              << std::endl;
}


void test_challenge_request_message() {
    // Arrange
    ChallengeRequestMessage original_message;
    original_message.to_username = "opponent";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    ChallengeRequestMessage deserialized_message = ChallengeRequestMessage::deserialize(serialized);

    // Assert
    std::cout << "ChallengeRequestMessage Test: " << (original_message.to_username == deserialized_message.to_username ? "Passed" : "Failed") << std::endl;
}
void test_player_list_message() {
    // Arrange
    PlayerListMessage original_message;
    original_message.players = {
        {"player1", 1500, true, "game123"},
        {"player2", 1600, false, ""},
        {"player3", 1700, true, "game456"}
    };

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    PlayerListMessage deserialized_message = PlayerListMessage::deserialize(serialized);

    // Assert
    bool number_of_players_match = original_message.players.size() == deserialized_message.players.size();
    bool players_match = true;
    for (size_t i = 0; i < original_message.players.size(); ++i) {
        if (original_message.players[i].username != deserialized_message.players[i].username ||
            original_message.players[i].elo != deserialized_message.players[i].elo ||
            original_message.players[i].in_game != deserialized_message.players[i].in_game ||
            original_message.players[i].game_id != deserialized_message.players[i].game_id) {
            players_match = false;
            break;
        }
    }

    std::cout << "PlayerListMessage Test: " 
              << (number_of_players_match && players_match ? "Passed" : "Failed") 
              << std::endl;
}

void test_challenge_response_message() {
    // Arrange
    ChallengeResponseMessage original_message;
    original_message.from_username = "challenger";
    original_message.response = ChallengeResponseMessage::Response::ACCEPTED;

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    ChallengeResponseMessage deserialized_message = ChallengeResponseMessage::deserialize(serialized);

    // Assert
    bool from_username_match = original_message.from_username == deserialized_message.from_username;
    bool response_match = original_message.response == deserialized_message.response;

    std::cout << "ChallengeResponseMessage Test: " 
              << (from_username_match && response_match ? "Passed" : "Failed") 
              << std::endl;
}

void test_request_spectate_message() {
    // Arrange
    RequestSpectateMessage original_message;
    original_message.username = "spectate_user";

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    RequestSpectateMessage deserialized_message = RequestSpectateMessage::deserialize(serialized);

    // Assert
    std::cout << "RequestSpectateMessage Test: " 
              << (original_message.username == deserialized_message.username ? "Passed" : "Failed") 
              << std::endl;
}

void test_spectate_success_message() {
    // Arrange
    SpectateSuccessMessage original_message;

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    SpectateSuccessMessage deserialized_message = SpectateSuccessMessage::deserialize(serialized);

    // Assert
    std::cout << "SpectateSuccessMessage Test: " 
              << (serialized.empty() ? "Passed" : "Failed") 
              << std::endl;
}

void test_spectate_failure_message() {
    // Arrange
    SpectateFailureMessage original_message;

    // Act
    std::vector<uint8_t> serialized = original_message.serialize();
    SpectateFailureMessage deserialized_message = SpectateFailureMessage::deserialize(serialized);

    // Assert
    std::cout << "SpectateFailureMessage Test: " 
              << (serialized.empty() ? "Passed" : "Failed") 
              << std::endl;
}

int main() {
    // test_register_message();
    // test_register_failure_message();
    // test_login_message();
    // test_login_success_message();
    // test_login_failure_message();
    // test_game_start_message();
    // test_move_message();
    // test_game_status_update_message();
    // test_game_end_message();

    // test_challenge_request_message();
    // test_game_status_update_message_extended();

     //test_player_list_message();
    // test_challenge_response_message();
    test_request_spectate_message();
    test_spectate_success_message();
    test_spectate_failure_message();
    return 0;
}
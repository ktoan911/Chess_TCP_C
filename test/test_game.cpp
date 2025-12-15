#include <iostream>
#include "../server/game_manager.hpp"
#include <vector>
#include <thread>

// Function to simulate a game
void simulateGame(GameManager &gameManager, const std::string &game_id, const std::vector<std::string> &moves, const std::string &game_name)
{
    std::cout << "\nSimulating " << game_name << ":" << std::endl;
    for (const auto &move : moves)
    {
        std::cout << game_name << " - Making move: " << move << std::endl;
        bool success = gameManager.makeMove(game_id, move);
        if (success)
        {
            std::cout << game_name << " - Move successful." << std::endl;
            std::string current_turn = gameManager.getGameCurrentTurn(game_id);
            std::cout << game_name << " - Current turn: " << current_turn << std::endl;
        }
        else
        {
            std::cout << game_name << " - Invalid move: " << move << std::endl;
        }

        std::cout << "--------------------------" << std::endl;

        // Check if the game is over
        if (gameManager.isGameOver(game_id))
        {
            std::cout << game_name << " - Game over!" << std::endl;

            std::string winner = gameManager.getGameWinner(game_id);
            if (winner == "<0>")
            {
                std::cout << game_name << " - Result: Draw" << std::endl;
            }
            else
            {
                std::cout << game_name << " - Winner: " << winner << std::endl;
            }

            std::cout << game_name << " - Reason: " << gameManager.getGameResultReason(game_id) << std::endl;
            break;
        }
    }
}

int main()
{
    GameManager &gameManager = GameManager::getInstance();

    // Create multiple games
    std::string initial_fen1 = "r1bkr3/pp4b1/7p/8/P1P2p2/8/1P1Pp1PP/R1B1K3 b - - 3 26";
    std::string game_id1 = gameManager.createGame("Alice", "Bob", initial_fen1);
    std::cout << "Game 1 created with ID: " << game_id1 << std::endl;

    std::string initial_fen2 = chess::constants::STARTPOS; // Standard starting position
    std::string game_id2 = gameManager.createGame("Charlie", "Diana");
    std::cout << "Game 2 created with ID: " << game_id2 << std::endl;

    // Define moves for each game
    std::vector<std::string> moves_game1 = {
        "f2e1", "g7d4",
        "d5c5", "e7d7",
        "b2b3", "d4f2",
        "e1f2", "e2e1Q",
        "f2f3", "e1f1"};

    std::vector<std::string> moves_game2 = {
        "e2e4", "e7e5",
        "g1f3", "b8c6",
        "f1c4", "g8f6",
        "d2d3", "f8c5",
        "c2c3", "d7d6"};

    // Create threads for each game simulation
    std::thread thread1(simulateGame, std::ref(gameManager), game_id1, moves_game1, "Game 1");
    std::thread thread2(simulateGame, std::ref(gameManager), game_id2, moves_game2, "Game 2");

    // Wait for both threads to finish
    thread1.join();
    thread2.join();

    // Verify multiple active games
    std::cout << "\nVerifying active games:" << std::endl;
    auto active_games = gameManager.getAllGames();
    std::cout << "Total active games: " << active_games.size() << std::endl;
    for (const auto &game : active_games)
    {
        std::cout << "Game ID: " << game->game_id
                  << ", Players: " << game->player_white_name << " vs " << game->player_black_name
                  << ", Current Turn: " << game->current_turn
                  << ", Half Moves: " << game->half_moves_count
                  << ", Game Over: " << (game->isGameOver() ? "Yes" : "No") << std::endl;
    }

    // Final game states
    std::cout << "\nFinal States:" << std::endl;
    for (const auto &game : active_games)
    {
        std::cout << "Game ID: " << game->game_id << std::endl;
        std::cout << "Final FEN: " << gameManager.getGameFen(game->game_id) << std::endl;
        std::cout << "Total Half Moves: " << gameManager.getGameHalfMovesCount(game->game_id) << std::endl;
        if (gameManager.isGameOver(game->game_id))
        {
            std::string winner = gameManager.getGameWinner(game->game_id);
            std::cout << "Winner: " << (winner.empty() ? "None" : winner) << std::endl;
            std::cout << "Reason: " << gameManager.getGameResultReason(game->game_id) << std::endl;
        }
        std::cout << "--------------------------" << std::endl;
    }

    // Clean up
    gameManager.removeGame(game_id1);
    gameManager.removeGame(game_id2);

    return 0;
}
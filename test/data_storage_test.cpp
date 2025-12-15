#include <iostream>

#include "../server/data_storage.hpp"

int main() {
    DataStorage &storage = DataStorage::getInstance();

    // Test 1: Register a new user
    std::cout << "Test 1: Register a new user 'alice'" << std::endl;
    bool result = storage.registerUser("alice");
    std::cout << "Expected: true, Got: " << std::boolalpha << result << std::endl;
    std::cout << "======================================" << std::endl;

    // Test 2: Register the same user again
    std::cout << "Test 2: Register the same user 'alice' again" << std::endl;
    result = storage.registerUser("alice");
    std::cout << "Expected: false, Got: " << std::boolalpha << result << std::endl;
    std::cout << "======================================" << std::endl;

    // Test 3: Validate existing user
    std::cout << "Test 3: Validate existing user 'alice'" << std::endl;
    bool isValid = storage.validateUser("alice");
    std::cout << "Expected: true, Got: " << std::boolalpha << isValid << std::endl;
    std::cout << "======================================" << std::endl;

    // Test 4: Validate non-existing user 'bob'
    std::cout << "Test 4: Validate non-existing user 'bob'" << std::endl;
    isValid = storage.validateUser("bob");
    std::cout << "Expected: false, Got: " << std::boolalpha << isValid << std::endl;
    std::cout << "======================================" << std::endl;

    // Test 5: Get ELO for existing user 'alice'
    std::cout << "Test 5: Get ELO for existing user 'alice'" << std::endl;
    uint16_t elo = storage.getUserELO("alice");
    std::cout << "Expected: 1200, Got: " << elo << std::endl;
    std::cout << "======================================" << std::endl;

    // Test 6: Get ELO for non-existing user 'bob'
    std::cout << "Test 6: Get ELO for non-existing user 'bob'" << std::endl;
    elo = storage.getUserELO("bob");
    std::cout << "Expected: 0, Got: " << elo << std::endl;
    std::cout << "======================================" << std::endl;

    return 0;
}
#include "../../include/BattleSystem.h"
#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include "../../include/UIManager.h"
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <streambuf>

// Mock streambuf to capture and simulate input/output
class MockStreamBuf : public std::streambuf {
public:
    MockStreamBuf() {}
    
    void setInput(const std::string& input) {
        inputBuffer = input;
        inputPos = 0;
    }
    
protected:
    int_type underflow() override {
        if (inputPos >= inputBuffer.length()) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(inputBuffer[inputPos]);
    }
    
    int_type uflow() override {
        if (inputPos >= inputBuffer.length()) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(inputBuffer[inputPos++]);
    }
    
    int_type pbackfail(int_type ch) override {
        if (inputPos > 0) {
            inputBuffer[--inputPos] = traits_type::to_char_type(ch);
            return ch;
        }
        return traits_type::eof();
    }
    
private:
    std::string inputBuffer;
    size_t inputPos = 0;
};

int main() {
    std::cout << "Battle System Test" << std::endl;
    
    // Create a player
    Player player(Player::CharacterClass::WARRIOR, "Hero");
    
    // Create UIManager
    UIManager uiManager;
    
    // Create BattleSystem
    BattleSystem battleSystem(&player, &uiManager);
    
    // Create some enemies
    std::vector<std::unique_ptr<Enemy>> enemies;
    enemies.push_back(std::make_unique<Enemy>("Goblin", 1));
    enemies.push_back(std::make_unique<Enemy>("Orc", 2));
    
    // Start battle
    battleSystem.startBattle(enemies);
    
    // Simulate player choosing "Attack"
    // We need to manually set the state in UIManager since we don't have a window/input loop
    uiManager.setBattleState(UIManager::BattleMenuState::MAIN_MENU);
    // Simulate pressing 'Enter' on the first option (Attack)
    uiManager.handleInput(2); 
    
    std::cout << "\n--- Battle Status ---" << std::endl;
    battleSystem.update();
    
    // Display final result
    BattleSystem::BattleResult result = battleSystem.getResult();
    switch (result) {
        case BattleSystem::BattleResult::PLAYER_WIN:
            std::cout << "Player wins!" << std::endl;
            break;
        case BattleSystem::BattleResult::PLAYER_LOSE:
            std::cout << "Player loses!" << std::endl;
            break;
        case BattleSystem::BattleResult::PLAYER_FLED:
            std::cout << "Player fled!" << std::endl;
            break;
        default:
            std::cout << "Battle is ongoing." << std::endl;
            break;
    }
    
    std::cout << "Battle system test completed successfully." << std::endl;
    
    return 0;
}

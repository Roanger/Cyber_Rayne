#include "../include/CharacterSelectionSystem.h"
#include "../include/Player.h"
#include <iostream>

int main() {
    std::cout << "Testing Character Selection System" << std::endl;
    
    CharacterSelectionSystem charSelect;
    charSelect.initialize();
    
    const auto& options = charSelect.getCharacterOptions();
    std::cout << "Available characters: " << options.size() << std::endl;
    
    for (size_t i = 0; i < options.size(); ++i) {
        const auto& option = options[i];
        std::cout << i << ". " << option.name << " - " << option.description << std::endl;
        std::cout << "   Starter Quest: " << option.starterQuest << std::endl;
    }
    
    // Test selecting a character
    charSelect.selectCharacter(0);
    Player* player = charSelect.createSelectedCharacter();
    
    if (player) {
        std::cout << "Created player: " << player->getName() << std::endl;
        std::cout << "Class: " << static_cast<int>(player->getCharacterClass()) << std::endl;
        std::cout << "Stats - HP: " << player->getHealth() << "/" << player->getMaxHealth() 
                  << ", MP: " << player->getMana() << "/" << player->getMaxMana() 
                  << ", STR: " << player->getStrength() 
                  << ", MAG: " << player->getMagic() 
                  << ", SPD: " << player->getSpeed() 
                  << ", DEF: " << player->getDefense() << std::endl;
        
        delete player;
    }
    
    return 0;
}

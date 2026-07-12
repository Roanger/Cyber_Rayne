#include "../../include/CharacterSelectionSystem.h"
#include "../../include/Player.h"
#include <iostream>
#include <stdexcept>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
}

int main() {
    CharacterSelectionSystem characterSelection;
    expect(characterSelection.initialize(), "character selection should initialize");

    const auto& options = characterSelection.getCharacterOptions();
    expect(options.size() == 3, "three character options should be available");
    expect(options[0].charClass == Player::CharacterClass::MAGE, "first option should be mage");
    expect(options[1].charClass == Player::CharacterClass::WARRIOR, "second option should be warrior");
    expect(options[2].charClass == Player::CharacterClass::ROGUE, "third option should be rogue");

    characterSelection.selectCharacter(1);
    auto player = characterSelection.createSelectedCharacter();
    expect(player != nullptr, "selected character should create a player");
    expect(player->getName() == options[1].name, "player name should match the selection");
    expect(player->getCharacterClass() == Player::CharacterClass::WARRIOR, "player class should match the selection");
    expect(player->getHealth() == player->getMaxHealth(), "new player should have full health");
    expect(player->getStrength() > player->getMagic(), "warrior strength should exceed magic");

    std::cout << "CharacterSelectionTest passed" << std::endl;
    return 0;
}

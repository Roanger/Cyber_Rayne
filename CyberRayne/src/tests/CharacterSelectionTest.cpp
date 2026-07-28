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
    expect(options[0].charClass == Player::CharacterClass::NETRUNNER, "first option should be netrunner");
    expect(options[1].charClass == Player::CharacterClass::SOLO, "second option should be solo");
    expect(options[2].charClass == Player::CharacterClass::INFILTRATOR, "third option should be infiltrator");

    characterSelection.selectCharacter(1);
    auto player = characterSelection.createSelectedCharacter();
    expect(player != nullptr, "selected character should create a player");
    expect(player->getName() == options[1].name, "player name should match the selection");
    expect(player->getCharacterClass() == Player::CharacterClass::SOLO, "player class should match the selection");
    expect(player->getHealth() == player->getMaxHealth(), "new player should have full health");
    expect(player->getStrength() > player->getMagic(), "solo strength should exceed magic");

    // Class name round-trip, since save/load and asset paths both depend on it.
    for (int i = 0; i < Player::CHARACTER_CLASS_COUNT; ++i) {
        auto original = static_cast<Player::CharacterClass>(i);
        Player::CharacterClass parsed{};
        expect(Player::classFromString(Player::classToString(original), parsed),
               "class name should parse back to a class");
        expect(parsed == original, "class name round-trip should preserve the class");
        expect(Player::isValidClassOrdinal(i), "in-range ordinal should be valid");
    }
    expect(!Player::isValidClassOrdinal(-1), "negative ordinal should be rejected");
    expect(!Player::isValidClassOrdinal(Player::CHARACTER_CLASS_COUNT), "out-of-range ordinal should be rejected");

    std::cout << "CharacterSelectionTest passed" << std::endl;
    return 0;
}

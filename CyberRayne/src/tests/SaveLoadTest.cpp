#include "../../include/SaveSystem.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/QuestLog.h"
#include "../../include/Quest.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <cstdio>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
} // namespace

int main() {
    const std::string savePath = "test_save.txt";

    // Create a party and quest log to save.
    auto party = Party::createDefaultParty();
    party->getLeader()->addGold(250);
    party->getLeader()->addExperience(50);

    // Level the leader up so the load path has real stat growth and ability
    // unlocks to reconstruct, not just a level-1 character.
    for (int i = 0; i < 4; ++i) {
        party->getLeader()->levelUp();
    }
    const int savedLevel = party->getLeader()->getLevel();
    const int savedMaxHealth = party->getLeader()->getMaxHealth();
    const int savedStrength = party->getLeader()->getStrength();
    const std::size_t savedAbilityCount = party->getLeader()->getAbilities().size();
    party->getLeader()->setHealth(37);

    auto questLog = std::make_unique<QuestLog>();
    auto quest = std::make_unique<Quest>(1, "Test Quest", "A test quest for serialization");
    quest->addObjective(QuestObjective("First objective"));
    quest->addObjective(QuestObjective("Second objective"));
    quest->setExperienceReward(200);
    quest->setGoldReward(100);
    quest->setState(Quest::State::ACTIVE);
    quest->completeObjective(0);
    questLog->addQuest(std::move(quest));

    // Save the game.
    SaveSystem saveSystem;
    expect(saveSystem.saveGame(savePath, party.get(), questLog.get()), "save should succeed");
    expect(saveSystem.saveExists(savePath), "save file should exist after saving");

    // Load into fresh objects.
    auto loadedParty = std::make_unique<Party>();
    auto loadedQuestLog = std::make_unique<QuestLog>();

    expect(saveSystem.loadGame(savePath, loadedParty.get(), loadedQuestLog.get()), "load should succeed");

    // Verify party.
    expect(loadedParty->size() == 3, "loaded party should have 3 members");

    const Player* leader = loadedParty->getLeader();
    expect(leader->getName() == "Rayne", "loaded party leader should be Rayne");
    expect(leader->getGold() >= 250, "loaded leader should have at least 250 gold");
    expect(leader->getLevel() == savedLevel, "loaded leader should keep its level");

    // Class must survive the round trip. It is stored as a bare enum ordinal,
    // so a corrupting change here would otherwise pass silently.
    expect(leader->getCharacterClass() == Player::CharacterClass::SOLO,
           "loaded leader should still be a Solo");
    for (std::size_t i = 0; i < loadedParty->size(); ++i) {
        expect(loadedParty->getMember(i)->getCharacterClass() ==
                   party->getMember(i)->getCharacterClass(),
               "every member's class should survive the round trip");
    }

    // Derived stats are rebuilt by replaying levelUp(). Before the loader was
    // fixed, setLevel() ran first and the replay loop never executed, so a
    // level-5 character loaded with level-1 stats.
    expect(leader->getMaxHealth() == savedMaxHealth, "max health should be rebuilt from the level");
    expect(leader->getStrength() == savedStrength, "strength should be rebuilt from the level");
    expect(leader->getHealth() == 37, "current health should be restored exactly");

    // Ability kit is derived from class and level, so it comes back with them.
    expect(leader->getAbilities().size() == savedAbilityCount,
           "loaded leader should know the same abilities");

    // Verify quest log.
    expect(loadedQuestLog->getQuests().size() == 1, "loaded quest log should have 1 quest");
    const Quest* loadedQuest = loadedQuestLog->getQuest(1);
    expect(loadedQuest != nullptr, "loaded quest should exist");
    expect(loadedQuest->getName() == "Test Quest", "loaded quest name should match");
    expect(loadedQuest->getState() == Quest::State::ACTIVE, "loaded quest should be ACTIVE");
    expect(loadedQuest->getObjectives().size() == 2, "loaded quest should have 2 objectives");
    expect(loadedQuest->getObjectives()[0].isCompleted(), "first objective should be completed");
    expect(!loadedQuest->getObjectives()[1].isCompleted(), "second objective should not be completed");
    expect(loadedQuest->getExperienceReward() == 200, "loaded quest XP reward should be 200");
    expect(loadedQuest->getGoldReward() == 100, "loaded quest gold reward should be 100");

    // Clean up the save file.
    std::remove(savePath.c_str());

    // Test loading non-existent file.
    auto emptyParty = std::make_unique<Party>();
    auto emptyQuestLog = std::make_unique<QuestLog>();
    expect(!saveSystem.loadGame("nonexistent_save.txt", emptyParty.get(), emptyQuestLog.get()),
           "loading non-existent file should fail");

    std::cout << "SaveLoadTest passed" << std::endl;
    return 0;
}

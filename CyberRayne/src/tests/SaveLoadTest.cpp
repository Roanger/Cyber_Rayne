#include "../../include/SaveSystem.h"
#include "../../include/Party.h"
#include "../../include/Player.h"
#include "../../include/QuestLog.h"
#include "../../include/Quest.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

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

    // Bag and gear. One piece is worn and its twin stays in the bag, to catch
    // a loader that confuses the two.
    party->getBag().add("stim_pack", 3);
    party->getBag().add("shock_baton", 2);
    party->getBag().add("kevlar_weave", 1);
    expect(party->equipFromBag(0, "shock_baton"), "the leader should equip the baton");
    expect(party->equipFromBag(1, "kevlar_weave"), "the second member should equip the armor");
    const int savedAttack = party->getLeader()->getEffectiveStrength();

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

    // Bag and gear come back exactly.
    expect(leader->getEquippedWeapon() && leader->getEquippedWeapon()->getId() == "shock_baton",
           "the leader's weapon should survive the round trip");
    expect(leader->getEquippedArmor() == nullptr, "an empty slot should stay empty");
    expect(leader->getEffectiveStrength() == savedAttack, "equipped gear should still count after loading");
    expect(loadedParty->getMember(1)->getEquippedArmor() &&
               loadedParty->getMember(1)->getEquippedArmor()->getId() == "kevlar_weave",
           "the second member's armor should survive the round trip");
    expect(loadedParty->getBag().count("stim_pack") == 3, "consumable counts should survive the round trip");
    expect(loadedParty->getBag().count("shock_baton") == 1, "the spare baton should still be in the bag");
    expect(loadedParty->getBag().count("kevlar_weave") == 0, "worn armor should not also be in the bag");
    expect(loadedParty->getBag().getStacks().size() == 2, "the bag should hold exactly what was saved");

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

    // --- World state, and saving into a directory that does not exist yet ---
    {
        namespace fs = std::filesystem;
        const fs::path dir = fs::temp_directory_path() / "cyberrayne_savetest" / "nested";
        fs::remove_all(dir.parent_path());
        const std::string nestedPath = (dir / "savegame.txt").string();

        SaveWorldState where;
        where.mapName = "Neon Alley";
        party->getLeader()->setPosition(15.0f, 8.0f);
        expect(saveSystem.saveGame(nestedPath, party.get(), questLog.get(), &where),
               "saving should create missing directories");
        expect(!fs::exists(nestedPath + ".tmp"), "the temporary file should not be left behind");

        auto party2 = std::make_unique<Party>();
        auto log2 = std::make_unique<QuestLog>();
        SaveWorldState loaded;
        loaded.mapName = "stale";
        expect(saveSystem.loadGame(nestedPath, party2.get(), log2.get(), &loaded), "load should succeed");
        expect(loaded.mapName == "Neon Alley", "the current map should survive the round trip");
        expect(loaded.hasBag, "a current save reports that it has a bag section");
        expect(party2->getLeader()->getX() == 15.0f && party2->getLeader()->getY() == 8.0f,
               "the leader's position should survive the round trip");

        // Overwriting an existing save works and leaves a complete file.
        where.mapName = "Corporate Plaza";
        expect(saveSystem.saveGame(nestedPath, party.get(), questLog.get(), &where), "re-saving should succeed");
        auto party3 = std::make_unique<Party>();
        auto log3 = std::make_unique<QuestLog>();
        expect(saveSystem.loadGame(nestedPath, party3.get(), log3.get(), &loaded) &&
                   loaded.mapName == "Corporate Plaza",
               "the newer save should replace the older one");

        // A save without world state loads, and reports no map.
        expect(saveSystem.saveGame(nestedPath, party.get(), questLog.get()), "saving without world state works");
        auto party4 = std::make_unique<Party>();
        auto log4 = std::make_unique<QuestLog>();
        loaded.mapName = "stale";
        expect(saveSystem.loadGame(nestedPath, party4.get(), log4.get(), &loaded) && loaded.mapName.empty(),
               "a save with no map should report an empty map name");

        // A save naming an item the catalog does not know still loads; the
        // unknown entries are dropped.
        {
            std::ifstream in(nestedPath);
            std::stringstream buffer;
            buffer << in.rdbuf();
            std::string text = buffer.str();
            const std::string from = "stack_0_id=stim_pack";
            const auto at = text.find(from);
            expect(at != std::string::npos, "the save should list the stim packs first");
            text.replace(at, from.size(), "stack_0_id=removed_item");
            const std::string weapon = "member_0_weapon=shock_baton";
            const auto wat = text.find(weapon);
            expect(wat != std::string::npos, "the save should record the leader's weapon");
            text.replace(wat, weapon.size(), "member_0_weapon=removed_item");
            std::ofstream out(nestedPath);
            out << text;
        }
        auto party5 = std::make_unique<Party>();
        auto log5 = std::make_unique<QuestLog>();
        expect(saveSystem.loadGame(nestedPath, party5.get(), log5.get()), "a save with unknown items still loads");
        expect(party5->size() == 3, "the party survives unknown items");
        expect(party5->getLeader()->getEquippedWeapon() == nullptr, "an unknown weapon leaves the slot bare");
        expect(party5->getBag().count("shock_baton") == 1 && party5->getBag().getStacks().size() == 1,
               "known bag entries survive next to unknown ones");

        // A save from before the bag existed is told apart from an empty bag.
        {
            std::ifstream in(nestedPath);
            std::stringstream buffer;
            buffer << in.rdbuf();
            std::string text = buffer.str();
            const auto bagAt = text.find("[BAG]");
            const auto questAt = text.find("[QUESTLOG]");
            expect(bagAt != std::string::npos && questAt != std::string::npos && bagAt < questAt,
                   "the bag section sits between the party and the quest log");
            text.erase(bagAt, questAt - bagAt);
            std::ofstream out(nestedPath);
            out << text;
        }
        auto party6 = std::make_unique<Party>();
        auto log6 = std::make_unique<QuestLog>();
        SaveWorldState old;
        old.hasBag = true;
        expect(saveSystem.loadGame(nestedPath, party6.get(), log6.get(), &old) && !old.hasBag,
               "a save with no bag section says so");
        expect(party6->getBag().isEmpty() && log6->getQuests().size() == 1,
               "and still loads the party and quests");

        fs::remove_all(dir.parent_path());
        expect(!SaveSystem::defaultSavePath().empty(), "there is always a default save path");
    }

    // Test loading non-existent file.
    auto emptyParty = std::make_unique<Party>();
    auto emptyQuestLog = std::make_unique<QuestLog>();
    expect(!saveSystem.loadGame("nonexistent_save.txt", emptyParty.get(), emptyQuestLog.get()),
           "loading non-existent file should fail");

    std::cout << "SaveLoadTest passed" << std::endl;
    return 0;
}

// Guards combat balance by simulating fights. Each fight is played many times
// by a simple but competent player: use the strongest affordable damage
// ability, focus the weakest enemy, never heal or use items. A real player has
// items and heals on top, so these win rates are a floor.
//
// This exists because the first tuning pass shipped a story boss that no
// class could beat and a Neon Alley ambush that wiped a level 1 party on
// arrival. Combat snowballs — small stat changes swing a fight from certain
// win to certain loss — so the numbers are checked rather than eyeballed.
#include "../../include/Ability.h"
#include "../../include/BattleSystem.h"
#include "../../include/Combatant.h"
#include "../../include/ContentFactory.h"
#include "../../include/EncounterTable.h"
#include "../../include/Enemy.h"
#include "../../include/Party.h"
#include "../../include/QuestLog.h"
#include "../../include/StoryDirector.h"
#include "../../include/World.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using Group = std::vector<std::pair<std::string, int>>;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "BalanceTest FAILED: " << message << std::endl;
        std::exit(1);
    }
}

std::unique_ptr<Party> makeParty(Player::CharacterClass rayneClass, int level) {
    auto party = std::make_unique<Party>();
    party->addMember(std::make_unique<Player>(rayneClass, "Rayne"));
    party->addMember(std::make_unique<Player>(Player::CharacterClass::NETRUNNER, "Cyph"));
    party->addMember(std::make_unique<Player>(Player::CharacterClass::INFILTRATOR, "Vash"));
    for (const auto& member : party->getMembers()) {
        while (member->getLevel() < level) member->levelUp();
    }
    return party;
}

// Returns true if the party wins.
bool playOnce(Player::CharacterClass rayneClass, int level, const Group& group) {
    auto party = makeParty(rayneClass, level);
    EncounterTable factory;
    std::vector<std::unique_ptr<Enemy>> foes;
    for (const auto& entry : group) {
        auto enemy = factory.spawnEnemy({entry.first, 1, entry.second});
        expect(enemy != nullptr, "unknown enemy type " + entry.first);
        foes.push_back(std::move(enemy));
    }

    BattleSystem battle(party.get());
    battle.startBattle(foes);
    for (int tick = 0; tick < 200000 && battle.getResult() == BattleSystem::BattleResult::ONGOING; ++tick) {
        if (battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION) {
            Combatant* actor = battle.getCurrentActor();

            int bestAbility = -1;
            int bestPower = actor->getEffectiveStrength();
            const auto& abilities = actor->getAbilities();
            for (std::size_t i = 0; i < abilities.size(); ++i) {
                const Ability* ability = abilities[i];
                if (ability->getKind() == Ability::Kind::DAMAGE && ability->canAfford(actor) &&
                    ability->computeMagnitude(actor) > bestPower) {
                    bestAbility = static_cast<int>(i);
                    bestPower = ability->computeMagnitude(actor);
                }
            }

            int target = -1;
            int lowest = 1 << 30;
            int index = 0;
            for (Combatant* enemy : battle.getEnemies()) {
                if (!enemy || !enemy->isAlive()) continue;
                if (enemy->getHealth() < lowest) {
                    lowest = enemy->getHealth();
                    target = index;
                }
                ++index;
            }

            if (bestAbility < 0 ||
                !battle.submitPlayerAction(BattleSystem::ActionRequest::ability(bestAbility, target))) {
                battle.submitPlayerAction(BattleSystem::ActionRequest::attack(target));
            }
        }
        battle.update(0.016f);
    }
    return battle.getResult() == BattleSystem::BattleResult::PLAYER_WIN;
}

int winPercent(Player::CharacterClass rayneClass, int level, const Group& group, int runs) {
    int wins = 0;
    for (int i = 0; i < runs; ++i) {
        if (playOnce(rayneClass, level, group)) ++wins;
    }
    return wins * 100 / runs;
}

std::string describe(const Group& group) {
    std::string text;
    for (const auto& entry : group) {
        text += (text.empty() ? "" : " + ") + entry.first + " L" + std::to_string(entry.second);
    }
    return text;
}

// Every class of Rayne must clear the bar.
void expectWinnable(const std::string& what, int level, const Group& group, int minPercent,
                    std::ostream& report) {
    const int runs = 60;
    for (int cls = 0; cls < Player::CHARACTER_CLASS_COUNT; ++cls) {
        const auto rayneClass = static_cast<Player::CharacterClass>(cls);
        const int percent = winPercent(rayneClass, level, group, runs);
        report << what << " [" << describe(group) << "] party L" << level << " as "
               << Player::classToString(rayneClass) << ": " << percent << "%\n";
        expect(percent >= minPercent,
               what + " should be winnable at level " + std::to_string(level) + " as " +
                   Player::classToString(rayneClass) + " (won " + std::to_string(percent) + "% of " +
                   describe(group) + ", need " + std::to_string(minPercent) + "%)");
    }
}

}  // namespace

int main() {
    // The battle code narrates to stdout; silence it and keep our own report.
    std::ostringstream sink;
    std::ostringstream report;
    std::streambuf* realOut = std::cout.rdbuf(sink.rdbuf());

    // --- Story fights, taken from the director so the test tracks the game ---
    QuestLog log;
    ContentFactory::createQuestChain(&log);
    StoryDirector story(&log, nullptr);
    StoryDirector::EncounterSpec thugs;
    StoryDirector::EncounterSpec enforcer;

    log.startQuest(StoryDirector::QUEST_DATA_CHIP);
    story.onMapEntered("Neon Alley");
    expect(story.getPendingEncounter("Neon Alley", thugs), "the thug fight should be pending");

    log.getQuest(StoryDirector::QUEST_DATA_CHIP)->setState(Quest::State::COMPLETED);
    log.startQuest(StoryDirector::QUEST_INTRUSION);
    story.onMapEntered("Corporate Plaza");
    expect(story.getPendingEncounter("Corporate Plaza", enforcer), "the Enforcer fight should be pending");

    // The thugs are met at level 1. The first quest's reward then carries the
    // party to level 3 before the plaza opens.
    expectWinnable("The thug fight", 1, thugs.enemies, 90, report);
    expectWinnable("The Enforcer fight", 3, enforcer.enemies, 90, report);

    {
        auto party = makeParty(Player::CharacterClass::NETRUNNER, 1);
        const int reward = log.getQuest(StoryDirector::QUEST_DATA_CHIP)->getExperienceReward();
        party->getLeader()->addExperience(reward);
        expect(party->getLeader()->getLevel() >= 3 ||
                   party->getLeader()->getExperienceToNextLevel() - party->getLeader()->getExperience() <= 60,
               "the first quest's XP should bring a level 1 party to, or within one fight of, level 3");
    }

    // --- Random encounters: the worst pair each district can roll ------------
    World world;
    world.initialize();
    struct District { World::BiomeType biome; const char* name; int partyLevel; };
    const District districts[] = {
        {World::BiomeType::NEO_KYOTO_CENTRAL, "Neo-Kyoto Central", 1},
        {World::BiomeType::NEON_ALLEY, "Neon Alley", 1},
        {World::BiomeType::CORPORATE_PLAZA, "Corporate Plaza", 3},
    };
    for (const District& district : districts) {
        const EncounterTable* table = world.getEncounterTable(district.biome);
        expect(table && !table->isEmpty(), std::string(district.name) + " should have an encounter table");

        // Ambushes are one or two enemies, so test every unordered pair.
        const auto& entries = table->getEntries();
        for (std::size_t i = 0; i < entries.size(); ++i) {
            for (std::size_t j = i; j < entries.size(); ++j) {
                const Group pair = {{entries[i].enemyTypeName, entries[i].level},
                                    {entries[j].enemyTypeName, entries[j].level}};
                expectWinnable(std::string(district.name) + " ambush", district.partyLevel, pair, 90, report);
            }
        }
    }

    std::cout.rdbuf(realOut);
    std::cout << report.str();
    std::cout << "BalanceTest passed" << std::endl;
    return 0;
}

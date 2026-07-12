#include "../../include/BattleSystem.h"
#include "../../include/Party.h"
#include "../../include/Enemy.h"
#include "../../include/Map.h"
#include "../../include/Player.h"
#include "../../include/Spell.h"
#include "../../include/StatusEffect.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
} // namespace

int main() {
    auto party = Party::createDefaultParty();
    expect(party->size() == 3, "default party should have 3 members");

    BattleSystem battle(party.get());

    std::vector<std::unique_ptr<Enemy>> enemies;
    enemies.push_back(std::make_unique<Enemy>("Training Drone", 1));
    const int initialEnemyHealth = enemies[0]->getHealth();

    battle.startBattle(enemies);
    expect(battle.getResult() == BattleSystem::BattleResult::ONGOING, "battle should start in progress");
    expect(battle.getState() == BattleSystem::State::FILLING_GAUGES, "battle should start filling gauges");
    expect(battle.getAllCombatants().size() == 4, "battle should have 3 party members + 1 enemy");

    // Before ATB fills, no action should be accepted.
    expect(!battle.submitPlayerAction(BattleSystem::BattleAction::ATTACK),
           "action should not be accepted before a player is ready");

    // Advance time until a player character becomes ready.
    // The leader (Warrior) has high speed and should fill first.
    int ticks = 0;
    while (battle.getState() != BattleSystem::State::WAITING_PLAYER_ACTION && ticks < 10000) {
        battle.update(0.016f);
        ++ticks;
    }
    expect(battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION,
           "a player character should become ready after ATB fills");
    expect(battle.getCurrentActor() != nullptr, "there should be a current actor");
    expect(battle.getCurrentActor()->isPlayer(), "the current actor should be a player");

    // Attacking should damage the enemy.
    expect(battle.submitPlayerAction(BattleSystem::BattleAction::ATTACK), "attack action should be accepted");
    expect(enemies[0]->getHealth() < initialEnemyHealth, "attack should damage the enemy");

    // After acting, ATB should reset and the actor should no longer be the current actor.
    expect(battle.getCurrentActor() == nullptr, "current actor should be cleared after action");
    expect(battle.getState() == BattleSystem::State::FILLING_GAUGES, "battle should resume filling gauges");

    // Defend should be accepted when a player is ready again.
    ticks = 0;
    while (battle.getState() != BattleSystem::State::WAITING_PLAYER_ACTION && ticks < 10000) {
        battle.update(0.016f);
        ++ticks;
    }
    expect(battle.submitPlayerAction(BattleSystem::BattleAction::DEFEND), "defend action should be accepted");

    // Status effect application: add a short poison to the enemy and tick it.
    // Use a magnitude high enough to overcome the enemy's defense.
    enemies[0]->addStatusEffect(std::make_unique<StatusEffect>(StatusEffect::Type::POISON, 0.5f, 20));
    const int prePoisonHealth = enemies[0]->getHealth();
    battle.update(0.3f);
    expect(enemies[0]->getHealth() < prePoisonHealth, "poison status effect should damage the enemy over time");

    // Test party defeat detection by killing all members.
    auto doomedParty = std::make_unique<Party>();
    auto doomed = std::make_unique<Player>(Player::CharacterClass::WARRIOR, "Doomed");
    doomed->setHealth(1);
    doomedParty->addMember(std::move(doomed));

    std::vector<std::unique_ptr<Enemy>> someEnemies;
    someEnemies.push_back(std::make_unique<Enemy>("Grunt", 1));

    BattleSystem losingBattle(doomedParty.get());
    losingBattle.startBattle(someEnemies);
    // Kill the lone party member outside of normal combat to test defeat detection.
    Player* doomedMember = doomedParty->getMember(0);
    expect(doomedMember != nullptr, "doomed party member should exist");
    doomedMember->takeDamage(999);
    losingBattle.update(0.016f);
    expect(losingBattle.getResult() == BattleSystem::BattleResult::PLAYER_LOSE,
           "battle should end in defeat when all party members fall");

    Map map("Test Map", 3, 3);
    expect(map.initialize(), "test map should initialize");
    map.addEnemy(std::make_unique<Enemy>("Respawn Test", 1));
    expect(map.getEnemies().size() == 1, "map should contain the spawned enemy");
    map.clearEnemies();
    expect(map.getEnemies().empty(), "clearing enemies should remove previous spawns");

    std::cout << "BattleSystemTest passed" << std::endl;
    return 0;
}

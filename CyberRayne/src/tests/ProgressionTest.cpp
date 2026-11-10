#include "../../include/Player.h"
#include "../../include/Party.h"
#include "../../include/Enemy.h"
#include "../../include/BattleSystem.h"
#include "../../include/Equipment.h"
#include "../../include/Item.h"
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
} // namespace

int main() {
    // Test XP and leveling.
    auto player = std::make_unique<Player>(Player::CharacterClass::SOLO, "TestHero");
    expect(player->getLevel() == 1, "player should start at level 1");
    expect(player->getExperience() == 0, "player should start with 0 XP");
    expect(player->getGold() == 0, "player should start with 0 gold");

    int xpForLevel1 = player->getExperienceToNextLevel();
    expect(xpForLevel1 > 0, "XP to next level should be positive");

    // Add enough XP to level up.
    player->addExperience(xpForLevel1);
    expect(player->getLevel() == 2, "player should be level 2 after gaining enough XP");
    expect(player->getMaxHealth() > 150, "max health should increase on level up (solo base 150)");

    // Test gold.
    player->addGold(100);
    expect(player->getGold() == 100, "player should have 100 gold");
    expect(player->spendGold(30), "spending 30 gold should succeed");
    expect(player->getGold() == 70, "player should have 70 gold remaining");
    expect(!player->spendGold(100), "spending more gold than available should fail");
    expect(player->getGold() == 70, "gold should be unchanged after failed spend");

    // Test equipment.
    auto weapon = std::make_unique<Weapon>("Plasma Sword", "A crackling blade of energy", 15);
    expect(weapon->getAttackBonus() == 15, "weapon attack bonus should be 15");
    expect(weapon->getType() == Item::ItemType::WEAPON, "weapon type should be WEAPON");

    player->equipWeapon(std::move(weapon));
    expect(player->getEquippedWeapon() != nullptr, "weapon should be equipped");
    expect(player->getEquippedWeapon()->getName() == "Plasma Sword", "equipped weapon name should match");

    auto armor = std::make_unique<Armor>("Kevlar Vest", "Lightweight protection", 10);
    expect(armor->getDefenseBonus() == 10, "armor defense bonus should be 10");
    expect(armor->getType() == Item::ItemType::ARMOR, "armor type should be ARMOR");

    player->equipArmor(std::move(armor));
    expect(player->getEquippedArmor() != nullptr, "armor should be equipped");
    expect(player->getEquippedArmor()->getName() == "Kevlar Vest", "equipped armor name should match");

    // Unequip and check.
    auto oldWeapon = player->unequipWeapon();
    expect(oldWeapon != nullptr, "unequipped weapon should be returned");
    expect(player->getEquippedWeapon() == nullptr, "weapon slot should be empty after unequip");

    // Test healing item.
    auto potion = std::make_unique<HealingItem>("Medkit", "Restores 50 HP", 50);
    player->setHealth(10);
    potion->use(player.get());
    expect(player->getHealth() == 60, "healing item should restore 50 HP");

    // Test battle XP/gold rewards.
    auto party = Party::createDefaultParty();
    BattleSystem battle(party.get());

    std::vector<std::unique_ptr<Enemy>> enemies;
    enemies.push_back(std::make_unique<Enemy>("Training Drone", 1));

    int enemyExp = enemies[0]->getExperienceReward();
    int enemyGold = enemies[0]->getGoldReward();
    expect(enemyExp > 0, "enemy should have positive XP reward");
    expect(enemyGold >= 0, "enemy should have non-negative gold reward");

    battle.startBattle(enemies);

    // Advance time until a player turn.
    int ticks = 0;
    while (battle.getState() != BattleSystem::State::WAITING_PLAYER_ACTION && ticks < 10000) {
        battle.update(0.016f);
        ++ticks;
    }
    expect(battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION, "player should get a turn");

    // Attack repeatedly until battle ends.
    while (battle.getResult() == BattleSystem::BattleResult::ONGOING) {
        if (battle.getState() == BattleSystem::State::WAITING_PLAYER_ACTION) {
            battle.submitPlayerAction(BattleSystem::BattleAction::ATTACK);
        }
        battle.update(0.016f);
        ++ticks;
        if (ticks > 50000) break;
    }

    expect(battle.getResult() == BattleSystem::BattleResult::PLAYER_WIN, "player should win the battle");
    expect(battle.getExperienceEarned() == enemyExp, "battle XP earned should match enemy XP reward");
    expect(battle.getGoldEarned() == enemyGold, "battle gold earned should match enemy gold reward");

    // At least one party member should have gained XP.
    bool someoneGainedXP = false;
    for (const auto& member : party->getMembers()) {
        if (member && member->getExperience() > 0) {
            someoneGainedXP = true;
            break;
        }
    }
    expect(someoneGainedXP, "at least one party member should have gained XP");

    std::cout << "ProgressionTest passed" << std::endl;
    return 0;
}

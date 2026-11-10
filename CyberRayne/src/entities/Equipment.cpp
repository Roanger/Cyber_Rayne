#include "../../include/Equipment.h"
#include "../../include/Player.h"
#include <iostream>

Weapon::Weapon(const std::string& name, const std::string& description, int attackBonus)
    : Item(name, ItemType::WEAPON, description), m_attackBonus(attackBonus) {}

void Weapon::use(Player* player) {
    // Weapons are equipped, not consumed.
    if (player) {
        std::cout << player->getName() << " examines " << getName() << " (ATK +" << m_attackBonus << ")" << std::endl;
    }
}

Armor::Armor(const std::string& name, const std::string& description, int defenseBonus)
    : Item(name, ItemType::ARMOR, description), m_defenseBonus(defenseBonus) {}

void Armor::use(Player* player) {
    // Armor is equipped, not consumed.
    if (player) {
        std::cout << player->getName() << " examines " << getName() << " (DEF +" << m_defenseBonus << ")" << std::endl;
    }
}

HealingItem::HealingItem(const std::string& name, const std::string& description, int healAmount)
    : Item(name, ItemType::CONSUMABLE, description), m_healAmount(healAmount) {}

void HealingItem::use(Player* player) {
    if (player) {
        player->heal(m_healAmount);
    }
}

bool HealingItem::canUseOn(const Player* target) const {
    return target && target->isAlive() && target->getHealth() < target->getMaxHealth();
}

ReviveItem::ReviveItem(const std::string& name, const std::string& description, int healthPercent)
    : Item(name, ItemType::CONSUMABLE, description), m_healthPercent(healthPercent) {}

void ReviveItem::use(Player* player) {
    if (!player || player->isAlive()) {
        return;
    }
    int health = player->getMaxHealth() * m_healthPercent / 100;
    if (health < 1) health = 1;
    player->setHealth(health);
    std::cout << player->getName() << " is back up with " << health << " HP." << std::endl;
}

bool ReviveItem::canUseOn(const Player* target) const {
    return target && !target->isAlive();
}

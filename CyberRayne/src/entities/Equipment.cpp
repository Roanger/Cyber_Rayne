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

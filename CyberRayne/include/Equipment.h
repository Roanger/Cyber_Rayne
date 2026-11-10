#pragma once

#include "Item.h"

// A weapon item that adds a strength bonus when equipped.
class Weapon : public Item {
public:
    Weapon(const std::string& name, const std::string& description, int attackBonus);
    int getAttackBonus() const override { return m_attackBonus; }

    // Using a weapon has no effect (it's equipment, not consumable).
    void use(Player* player) override;

private:
    int m_attackBonus;
};

// An armor item that adds a defense bonus when equipped.
class Armor : public Item {
public:
    Armor(const std::string& name, const std::string& description, int defenseBonus);
    int getDefenseBonus() const override { return m_defenseBonus; }

    // Using armor has no effect (it's equipment, not consumable).
    void use(Player* player) override;

private:
    int m_defenseBonus;
};

// A consumable healing item.
class HealingItem : public Item {
public:
    HealingItem(const std::string& name, const std::string& description, int healAmount);
    int getHealAmount() const { return m_healAmount; }

    void use(Player* player) override;
    // Only the living and the wounded: it cannot raise the fallen.
    bool canUseOn(const Player* target) const override;

private:
    int m_healAmount;
};

// A consumable that gets a downed ally back up.
class ReviveItem : public Item {
public:
    // healthPercent is how much of max HP the ally returns with, 1-100.
    ReviveItem(const std::string& name, const std::string& description, int healthPercent);

    void use(Player* player) override;
    bool canUseOn(const Player* target) const override;

private:
    int m_healthPercent;
};

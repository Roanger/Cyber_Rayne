#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Combatant.h"


class Enemy : public Combatant {
public:
    Enemy(const std::string& name, int level);
    ~Enemy();

    // Combatant interface
    const std::string& getName() const override { return m_name; }
    Allegiance getAllegiance() const override { return Allegiance::ENEMY; }
    int getLevel() const override { return m_level; }
    int getHealth() const override { return m_health; }
    int getMaxHealth() const override { return m_maxHealth; }
    int getMana() const override { return m_mana; }
    int getMaxMana() const override { return m_maxMana; }
    int getStrength() const override { return m_strength; }
    int getMagic() const override { return m_magic; }
    int getSpeed() const override { return m_speed; }
    int getDefense() const override { return m_defense; }
    bool isAlive() const override { return m_health > 0; }

    // Enemy-specific getters
    int getExperienceReward() const { return m_experienceReward; }
    int getGoldReward() const { return m_goldReward; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }

    // Setters
    void setHealth(int health) override { m_health = health; }
    void setMana(int mana) override { m_mana = mana; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }

    // Combat functions
    void takeDamage(int damage) override;
    void heal(int amount) override;
    int calculateDamage() const;

    // AI decision making
    enum class EnemyAction {
        ATTACK,
        MAGIC,
        DEFEND
    };
    
    EnemyAction decideAction() const;

protected:
    virtual void initializeStats();

    std::string m_name;
    int m_level;
    int m_health;
    int m_maxHealth;
    int m_mana;
    int m_maxMana;
    int m_strength;
    int m_magic;
    int m_speed;
    int m_defense;
    int m_experienceReward;
    int m_goldReward;
    float m_x;
    float m_y;
};

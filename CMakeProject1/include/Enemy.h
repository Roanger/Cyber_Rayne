#pragma once

#include <string>
#include <vector>
#include <memory>

class Spell;

class Enemy {
public:
    Enemy(const std::string& name, int level);
    ~Enemy();

    // Getters
    const std::string& getName() const { return m_name; }
    int getLevel() const { return m_level; }
    int getHealth() const { return m_health; }
    int getMana() const { return m_mana; }
    int getStrength() const { return m_strength; }
    int getMagic() const { return m_magic; }
    int getSpeed() const { return m_speed; }
    int getDefense() const { return m_defense; }
    int getExperienceReward() const { return m_experienceReward; }
    int getGoldReward() const { return m_goldReward; }
    int getMaxHealth() const { return m_maxHealth; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }

    // Setters
    void setHealth(int health) { m_health = health; }
    void setMana(int mana) { m_mana = mana; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }

    // Combat functions
    void takeDamage(int damage);
    bool isAlive() const { return m_health > 0; }
    int calculateDamage() const;

    // AI decision making
    enum class EnemyAction {
        ATTACK,
        MAGIC,
        DEFEND
    };
    
    EnemyAction decideAction() const;

    // Spell management
    void addSpell(std::shared_ptr<Spell> spell);
    const std::vector<std::shared_ptr<Spell>>& getSpells() const { return m_spells; }

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

    std::vector<std::shared_ptr<Spell>> m_spells;
};

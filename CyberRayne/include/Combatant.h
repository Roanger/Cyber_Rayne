#pragma once

#include <string>
#include <vector>
#include <memory>

#include "StatusEffect.h"

// Common interface for all battle participants (player characters and enemies).
class Combatant {
public:
    enum class Allegiance {
        PLAYER,
        ENEMY
    };

    virtual ~Combatant() = default;

    // Identity
    virtual const std::string& getName() const = 0;
    virtual Allegiance getAllegiance() const = 0;
    bool isPlayer() const { return getAllegiance() == Allegiance::PLAYER; }
    bool isEnemy() const { return getAllegiance() == Allegiance::ENEMY; }

    // Stats
    virtual int getHealth() const = 0;
    virtual int getMaxHealth() const = 0;
    virtual int getMana() const = 0;
    virtual int getMaxMana() const = 0;
    virtual int getStrength() const = 0;
    virtual int getMagic() const = 0;
    virtual int getSpeed() const = 0;
    virtual int getDefense() const = 0;
    virtual int getLevel() const = 0;

    // Lifecycle
    virtual bool isAlive() const = 0;
    virtual void takeDamage(int damage) = 0;
    virtual void heal(int amount) = 0;
    virtual void setHealth(int health) = 0;
    virtual void setMana(int mana) = 0;

    // Status effects
    virtual void addStatusEffect(std::unique_ptr<StatusEffect> effect);
    virtual void updateStatusEffects(float deltaTime);
    virtual void clearExpiredStatusEffects();
    const std::vector<std::unique_ptr<StatusEffect>>& getStatusEffects() const { return m_statusEffects; }

    // Active Time Battle gauge (0..1)
    float getAtbGauge() const { return m_atbGauge; }
    void setAtbGauge(float gauge) { m_atbGauge = gauge; }
    void fillAtb(float amount) { m_atbGauge += amount; if (m_atbGauge > 1.0f) m_atbGauge = 1.0f; }
    bool isAtbFull() const { return m_atbGauge >= 1.0f; }
    void resetAtb() { m_atbGauge = 0.0f; }

    // Defend flag for the current turn
    bool isDefending() const { return m_isDefending; }
    void setDefending(bool defending) { m_isDefending = defending; }

protected:
    float m_atbGauge = 0.0f;
    bool m_isDefending = false;
    std::vector<std::unique_ptr<StatusEffect>> m_statusEffects;
};

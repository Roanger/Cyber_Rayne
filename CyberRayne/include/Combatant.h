#pragma once

#include <string>
#include <vector>
#include <memory>

#include "StatusEffect.h"

class Ability;

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

    // Equipment contribution to combat stats. Zero for combatants that can't
    // equip anything, so the effective-stat math below stays uniform.
    virtual int getEquipmentAttackBonus() const { return 0; }
    virtual int getEquipmentDefenseBonus() const { return 0; }

    // Abilities. Held on the base class so BattleSystem can resolve an action
    // without knowing whether the actor is a party member or an enemy.
    // Pointers are non-owning; AbilityRegistry owns the ability objects.
    void learnAbility(const Ability* ability);
    const std::vector<const Ability*>& getAbilities() const { return m_abilities; }
    bool knowsAbility(const Ability* ability) const;

    // Status effects
    virtual void addStatusEffect(std::unique_ptr<StatusEffect> effect);
    virtual void updateStatusEffects(float deltaTime);
    virtual void clearExpiredStatusEffects();
    const std::vector<std::unique_ptr<StatusEffect>>& getStatusEffects() const { return m_statusEffects; }
    bool hasStatusEffect(StatusEffect::Type type) const;

    // Combat-facing stats: base stat + equipment + active status modifiers.
    // Battle logic should read these rather than the raw getters.
    int getEffectiveStrength() const;
    int getEffectiveDefense() const;
    float getAtbSpeedMultiplier() const;
    bool isStunned() const;

    // Shared mitigation so players and enemies take damage on the same curve.
    // Defense halves incoming damage rather than subtracting outright, and at
    // least MIN_DAMAGE always lands — otherwise a high-defense enemy such as
    // StoneGolem (40 + 5/level) is simply immune to early-game attacks.
    static constexpr int MIN_DAMAGE = 1;
    static int computeMitigatedDamage(int rawDamage, int effectiveDefense) {
        int mitigated = rawDamage - effectiveDefense / 2;
        return mitigated < MIN_DAMAGE ? MIN_DAMAGE : mitigated;
    }

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
    std::vector<const Ability*> m_abilities;
};

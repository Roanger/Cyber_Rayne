#pragma once

#include <string>

class Combatant;

// Abstract base for status effects that can be applied to a Combatant.
class StatusEffect {
public:
    enum class Type {
        POISON,       // Damage over time
        STUN,         // Skip next turn
        DEFENSE_UP,   // Increases defense
        DEFENSE_DOWN, // Reduces defense
        ATTACK_UP,    // Increases attack damage
        ATTACK_DOWN,  // Reduces attack damage
        SLOW,         // Reduces ATB fill rate
        REGEN         // Heal over time
    };

    // Seconds between damage/heal applications. Magnitude is per tick, not
    // per frame — applying it every frame made a magnitude-20 poison deal
    // roughly 1200 damage per second at 60fps.
    static constexpr float TICK_INTERVAL = 1.0f;

    // ATB fill multiplier applied while SLOW is active.
    static constexpr float SLOW_ATB_MULTIPLIER = 0.5f;

    StatusEffect(Type type, float durationSeconds, int magnitude = 0);
    virtual ~StatusEffect() = default;

    Type getType() const { return m_type; }
    float getRemainingTime() const { return m_remainingTime; }
    int getMagnitude() const { return m_magnitude; }
    bool isExpired() const { return m_remainingTime <= 0.0f; }

    // Advance the effect clock. Returns true while the effect is still active.
    bool update(float deltaTime);

    // Apply the per-tick effect (damage, heal, etc.). Only fires once the
    // accumulated time crosses TICK_INTERVAL.
    virtual void onTick(Combatant* target, float deltaTime);

    // Apply one-time or per-turn modifiers. Called when the affected combatant acts.
    virtual void onApply(Combatant* target);
    virtual void onRemove(Combatant* target);

    // Signed modifier this effect contributes to the given stat, so callers
    // don't have to switch on Type themselves.
    int getStrengthModifier() const;
    int getDefenseModifier() const;

protected:
    Type m_type;
    float m_remainingTime;
    int m_magnitude;
    float m_tickAccumulator = 0.0f;
};

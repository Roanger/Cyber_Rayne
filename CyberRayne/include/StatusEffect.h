#pragma once

#include <string>

class Combatant;

// Abstract base for status effects that can be applied to a Combatant.
class StatusEffect {
public:
    enum class Type {
        POISON,     // Damage over time
        STUN,       // Skip next turn
        DEFENSE_UP, // Increases defense
        ATTACK_UP,  // Increases attack damage
        SLOW,       // Reduces speed / ATB fill
        REGEN       // Heal over time
    };

    StatusEffect(Type type, float durationSeconds, int magnitude = 0);
    virtual ~StatusEffect() = default;

    Type getType() const { return m_type; }
    float getRemainingTime() const { return m_remainingTime; }
    int getMagnitude() const { return m_magnitude; }

    // Called once per battle tick. Returns true if the effect is still active.
    bool update(float deltaTime);

    // Apply the per-tick effect (damage, heal, etc.). Called when the effect ticks.
    virtual void onTick(Combatant* target, float deltaTime);

    // Apply one-time or per-turn modifiers. Called when the affected combatant acts.
    virtual void onApply(Combatant* target);
    virtual void onRemove(Combatant* target);

protected:
    Type m_type;
    float m_remainingTime;
    int m_magnitude;
};

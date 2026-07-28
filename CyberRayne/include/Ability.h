#pragma once

#include <string>
#include <vector>

#include "StatusEffect.h"

class Combatant;

// A single combat action beyond the basic attack: an attack program, a heal, a
// buff, or a debuff.
//
// Abilities operate purely on Combatant, so party members and enemies run
// through the same execution path. The older Spell type took concrete Player*
// and Enemy* arguments, which forced downcasts in BattleSystem and made enemy
// casting impossible.
class Ability {
public:
    enum class Kind {
        DAMAGE,
        HEAL,
        BUFF,    // Applies a positive status to allies
        DEBUFF   // Applies a negative status to enemies
    };

    enum class Target {
        SELF,
        SINGLE_ALLY,
        ALL_ALLIES,
        SINGLE_ENEMY,
        ALL_ENEMIES
    };

    // Which stat the ability's power scales from.
    enum class Scaling {
        PHYSICAL,  // effective strength
        TECH       // magic
    };

    Ability() = default;
    Ability(std::string id,
            std::string name,
            std::string description,
            Kind kind,
            Target target,
            Scaling scaling,
            int manaCost,
            int power,
            int unlockLevel);

    const std::string& getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    const std::string& getDescription() const { return m_description; }
    Kind getKind() const { return m_kind; }
    Target getTarget() const { return m_target; }
    Scaling getScaling() const { return m_scaling; }
    int getManaCost() const { return m_manaCost; }
    int getPower() const { return m_power; }
    int getUnlockLevel() const { return m_unlockLevel; }

    // Optional status effect applied to each resolved target.
    bool appliesStatus() const { return m_appliesStatus; }
    StatusEffect::Type getStatusType() const { return m_statusType; }
    float getStatusDuration() const { return m_statusDuration; }
    int getStatusMagnitude() const { return m_statusMagnitude; }
    Ability& withStatus(StatusEffect::Type type, float duration, int magnitude);

    // True when the ability resolves against enemies rather than allies. Used
    // by the UI to decide which side to offer for manual targeting.
    bool targetsEnemies() const;
    // True when the player must pick one target from a list.
    bool needsTargetChoice() const;

    bool canAfford(const Combatant* caster) const;

    // Compute the damage or healing this ability would do, before the target's
    // mitigation. Public so the UI can preview numbers.
    int computeMagnitude(const Combatant* caster) const;

    // Spend the mana cost. Call only after canAfford() passes and the action is
    // committed. Kept separate from execute() so a rejected action never
    // charges the caster.
    void payCost(Combatant* caster) const;

    // Apply the ability to already-resolved targets.
    void execute(Combatant* caster, const std::vector<Combatant*>& targets) const;

private:
    std::string m_id;
    std::string m_name;
    std::string m_description;
    Kind m_kind = Kind::DAMAGE;
    Target m_target = Target::SINGLE_ENEMY;
    Scaling m_scaling = Scaling::TECH;
    int m_manaCost = 0;
    int m_power = 0;
    int m_unlockLevel = 1;

    bool m_appliesStatus = false;
    StatusEffect::Type m_statusType = StatusEffect::Type::POISON;
    float m_statusDuration = 0.0f;
    int m_statusMagnitude = 0;
};

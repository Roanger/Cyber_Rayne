#include "../../include/Combatant.h"
#include "../../include/StatusEffect.h"
#include "../../include/Ability.h"

#include <algorithm>

void Combatant::addStatusEffect(std::unique_ptr<StatusEffect> effect) {
    if (!effect) {
        return;
    }
    effect->onApply(this);
    m_statusEffects.push_back(std::move(effect));
}

void Combatant::learnAbility(const Ability* ability) {
    if (!ability || knowsAbility(ability)) {
        return;
    }
    m_abilities.push_back(ability);
}

bool Combatant::knowsAbility(const Ability* ability) const {
    return std::find(m_abilities.begin(), m_abilities.end(), ability) != m_abilities.end();
}

void Combatant::updateStatusEffects(float deltaTime) {
    for (auto& effect : m_statusEffects) {
        if (effect) {
            effect->onTick(this, deltaTime);
            // Advancing the clock is what makes effects expire. Without this
            // call the remaining time never changes and effects last forever.
            effect->update(deltaTime);
        }
    }
    clearExpiredStatusEffects();
}

void Combatant::clearExpiredStatusEffects() {
    for (auto it = m_statusEffects.begin(); it != m_statusEffects.end();) {
        if (!(*it) || (*it)->isExpired()) {
            if (*it) {
                (*it)->onRemove(this);
            }
            it = m_statusEffects.erase(it);
        } else {
            ++it;
        }
    }
}

bool Combatant::hasStatusEffect(StatusEffect::Type type) const {
    for (const auto& effect : m_statusEffects) {
        if (effect && effect->getType() == type) {
            return true;
        }
    }
    return false;
}

int Combatant::getEffectiveStrength() const {
    int total = getStrength() + getEquipmentAttackBonus();
    for (const auto& effect : m_statusEffects) {
        if (effect) {
            total += effect->getStrengthModifier();
        }
    }
    return total < 0 ? 0 : total;
}

int Combatant::getEffectiveDefense() const {
    int total = getDefense() + getEquipmentDefenseBonus();
    for (const auto& effect : m_statusEffects) {
        if (effect) {
            total += effect->getDefenseModifier();
        }
    }
    return total < 0 ? 0 : total;
}

float Combatant::getAtbSpeedMultiplier() const {
    return hasStatusEffect(StatusEffect::Type::SLOW) ? StatusEffect::SLOW_ATB_MULTIPLIER : 1.0f;
}

bool Combatant::isStunned() const {
    return hasStatusEffect(StatusEffect::Type::STUN);
}

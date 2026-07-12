#include "../../include/Combatant.h"
#include "../../include/StatusEffect.h"

void Combatant::addStatusEffect(std::unique_ptr<StatusEffect> effect) {
    if (!effect) {
        return;
    }
    effect->onApply(this);
    m_statusEffects.push_back(std::move(effect));
}

void Combatant::updateStatusEffects(float deltaTime) {
    for (auto& effect : m_statusEffects) {
        if (effect) {
            effect->onTick(this, deltaTime);
        }
    }
    clearExpiredStatusEffects();
}

void Combatant::clearExpiredStatusEffects() {
    for (auto it = m_statusEffects.begin(); it != m_statusEffects.end();) {
        if (!(*it) || (*it)->getRemainingTime() <= 0.0f) {
            if (*it) {
                (*it)->onRemove(this);
            }
            it = m_statusEffects.erase(it);
        } else {
            ++it;
        }
    }
}

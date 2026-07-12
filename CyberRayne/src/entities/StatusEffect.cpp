#include "../../include/StatusEffect.h"
#include "../../include/Combatant.h"
#include <iostream>

StatusEffect::StatusEffect(Type type, float durationSeconds, int magnitude)
    : m_type(type), m_remainingTime(durationSeconds), m_magnitude(magnitude) {}

bool StatusEffect::update(float deltaTime) {
    m_remainingTime -= deltaTime;
    return m_remainingTime > 0.0f;
}

void StatusEffect::onTick(Combatant* target, float /*deltaTime*/) {
    if (!target || !target->isAlive()) {
        return;
    }

    switch (m_type) {
        case Type::POISON:
            if (m_magnitude > 0) {
                std::cout << target->getName() << " takes " << m_magnitude << " poison damage!" << std::endl;
                target->takeDamage(m_magnitude);
            }
            break;
        case Type::REGEN:
            if (m_magnitude > 0) {
                std::cout << target->getName() << " regenerates " << m_magnitude << " HP!" << std::endl;
                target->heal(m_magnitude);
            }
            break;
        default:
            break;
    }
}

void StatusEffect::onApply(Combatant* target) {
    if (!target) return;
    switch (m_type) {
        case Type::DEFENSE_UP:
            std::cout << target->getName() << "'s defense rises!" << std::endl;
            break;
        case Type::ATTACK_UP:
            std::cout << target->getName() << "'s attack rises!" << std::endl;
            break;
        case Type::SLOW:
            std::cout << target->getName() << " is slowed!" << std::endl;
            break;
        case Type::STUN:
            std::cout << target->getName() << " is stunned!" << std::endl;
            break;
        default:
            break;
    }
}

void StatusEffect::onRemove(Combatant* target) {
    if (!target) return;
    switch (m_type) {
        case Type::DEFENSE_UP:
            std::cout << target->getName() << "'s defense returns to normal." << std::endl;
            break;
        case Type::ATTACK_UP:
            std::cout << target->getName() << "'s attack returns to normal." << std::endl;
            break;
        case Type::SLOW:
            std::cout << target->getName() << " is no longer slowed." << std::endl;
            break;
        case Type::STUN:
            std::cout << target->getName() << " is no longer stunned." << std::endl;
            break;
        default:
            break;
    }
}

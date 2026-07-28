#include "../../include/Ability.h"
#include "../../include/Combatant.h"

#include <iostream>
#include <memory>
#include <utility>

Ability::Ability(std::string id,
                 std::string name,
                 std::string description,
                 Kind kind,
                 Target target,
                 Scaling scaling,
                 int manaCost,
                 int power,
                 int unlockLevel)
    : m_id(std::move(id)),
      m_name(std::move(name)),
      m_description(std::move(description)),
      m_kind(kind),
      m_target(target),
      m_scaling(scaling),
      m_manaCost(manaCost),
      m_power(power),
      m_unlockLevel(unlockLevel) {}

Ability& Ability::withStatus(StatusEffect::Type type, float duration, int magnitude) {
    m_appliesStatus = true;
    m_statusType = type;
    m_statusDuration = duration;
    m_statusMagnitude = magnitude;
    return *this;
}

bool Ability::targetsEnemies() const {
    return m_target == Target::SINGLE_ENEMY || m_target == Target::ALL_ENEMIES;
}

bool Ability::needsTargetChoice() const {
    return m_target == Target::SINGLE_ENEMY || m_target == Target::SINGLE_ALLY;
}

bool Ability::canAfford(const Combatant* caster) const {
    return caster && caster->getMana() >= m_manaCost;
}

int Ability::computeMagnitude(const Combatant* caster) const {
    if (!caster) {
        return m_power;
    }

    int stat = (m_scaling == Scaling::PHYSICAL) ? caster->getEffectiveStrength()
                                                : caster->getMagic();
    return m_power + stat / 2;
}

void Ability::payCost(Combatant* caster) const {
    if (!caster || m_manaCost <= 0) {
        return;
    }
    caster->setMana(caster->getMana() - m_manaCost);
}

void Ability::execute(Combatant* caster, const std::vector<Combatant*>& targets) const {
    if (!caster) {
        return;
    }

    std::cout << caster->getName() << " uses " << m_name << "!" << std::endl;

    const int magnitude = computeMagnitude(caster);

    for (Combatant* target : targets) {
        if (!target || !target->isAlive()) {
            continue;
        }

        switch (m_kind) {
            case Kind::DAMAGE:
                target->takeDamage(magnitude);
                break;
            case Kind::HEAL:
                target->heal(magnitude);
                break;
            case Kind::BUFF:
            case Kind::DEBUFF:
                // The status carries the whole effect for these kinds.
                break;
        }

        if (m_appliesStatus) {
            target->addStatusEffect(std::make_unique<StatusEffect>(
                m_statusType, m_statusDuration, m_statusMagnitude));
        }
    }
}

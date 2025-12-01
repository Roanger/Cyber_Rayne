#include "../../include/Spell.h"
#include "../../include/Player.h"
#include "../../include/Enemy.h"
#include <iostream>

Spell::Spell(const std::string& name, SpellType type, Element element, int manaCost, int power)
    : m_name(name), m_type(type), m_element(element), m_manaCost(manaCost), m_power(power) {}

Spell::~Spell() {}

void Spell::cast(Player* caster, Enemy* target) {
    if (caster->getMana() < m_manaCost) {
        std::cout << caster->getName() << " does not have enough mana to cast " << m_name << "!" << std::endl;
        return;
    }

    caster->setMana(caster->getMana() - m_manaCost);
    std::cout << caster->getName() << " casts " << m_name << " on " << target->getName() << std::endl;

    if (m_type == SpellType::DAMAGE) {
        // Simple damage formula: Power + Magic * 0.5
        int damage = m_power + static_cast<int>(caster->getMagic() * 0.5f);
        // Apply elemental weakness/resistance here if implemented
        
        std::cout << "  " << m_name << " deals " << damage << " damage!" << std::endl;
        target->takeDamage(damage);
    } else if (m_type == SpellType::DEBUFF) {
        std::cout << "  " << m_name << " weakens " << target->getName() << "!" << std::endl;
        // TODO: Implement status effects
    }
}

void Spell::cast(Player* caster, Player* target) {
    if (caster->getMana() < m_manaCost) {
        std::cout << caster->getName() << " does not have enough mana to cast " << m_name << "!" << std::endl;
        return;
    }

    caster->setMana(caster->getMana() - m_manaCost);
    std::cout << caster->getName() << " casts " << m_name << " on " << target->getName() << std::endl;

    if (m_type == SpellType::HEAL) {
        int healAmount = m_power + static_cast<int>(caster->getMagic() * 0.5f);
        std::cout << "  " << m_name << " heals " << healAmount << " HP!" << std::endl;
        target->heal(healAmount);
    } else if (m_type == SpellType::BUFF) {
        std::cout << "  " << m_name << " strengthens " << target->getName() << "!" << std::endl;
        // TODO: Implement status effects
    }
}

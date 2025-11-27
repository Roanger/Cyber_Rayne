#pragma once

#include <string>

class Player;
class Enemy;

class Spell {
public:
    enum class SpellType {
        DAMAGE,
        HEAL,
        BUFF,
        DEBUFF
    };

    enum class Element {
        FIRE,
        ICE,
        LIGHTNING,
        EARTH,
        WIND,
        HOLY,
        DARK,
        NONE
    };

    Spell(const std::string& name, SpellType type, Element element, int manaCost, int power);
    ~Spell();

    // Getters
    const std::string& getName() const { return m_name; }
    SpellType getType() const { return m_type; }
    Element getElement() const { return m_element; }
    int getManaCost() const { return m_manaCost; }
    int getPower() const { return m_power; }

    // Spell effects
    void cast(Player* caster, Enemy* target);
    void cast(Player* caster, Player* target);

private:
    std::string m_name;
    SpellType m_type;
    Element m_element;
    int m_manaCost;
    int m_power;
};

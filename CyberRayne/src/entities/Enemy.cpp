#include "../../include/Enemy.h"
#include "../../include/Spell.h"
#include <iostream>
#include <algorithm>
#include <random>

Enemy::Enemy(const std::string& name, int level)
    : m_name(name), m_level(level), m_x(0.0f), m_y(0.0f) {
    initializeStats();
}

Enemy::~Enemy() {}

void Enemy::initializeStats() {
    // Initialize stats based on enemy type and level
    // This is a simplified implementation
    m_maxHealth = 50 + (m_level * 10);
    m_health = m_maxHealth;
    m_maxMana = 30 + (m_level * 5);
    m_mana = m_maxMana;
    m_strength = 8 + (m_level * 1);
    m_magic = 5 + (m_level * 1);
    m_speed = 10 + (m_level * 1);
    m_defense = 3 + (m_level * 1);
    m_experienceReward = 10 + (m_level * 2);
    m_goldReward = 5 + (m_level * 1);
    
    std::cout << "Enemy " << m_name << " initialized with level " << m_level << std::endl;
}

void Enemy::takeDamage(int damage) {
    int actualDamage = std::max(0, damage - m_defense);
    m_health -= actualDamage;
    
    if (m_health <= 0) {
        m_health = 0;
        std::cout << m_name << " has been defeated!" << std::endl;
    } else {
        std::cout << m_name << " takes " << actualDamage << " damage. "
                  << "Health: " << m_health << "/" << m_maxHealth << std::endl;
    }
}

int Enemy::calculateDamage() const {
    // Simple damage calculation
    return m_strength;
}

Enemy::EnemyAction Enemy::decideAction() const {
    // Simple AI decision making
    // In a real implementation, this would be more complex
    if (!m_spells.empty() && m_mana >= 10 && (rand() % 100) < 30) {  // 30% chance to cast a spell
        return EnemyAction::MAGIC;
    }
    return EnemyAction::ATTACK;
}

void Enemy::addSpell(std::shared_ptr<Spell> spell) {
    m_spells.push_back(spell);
    std::cout << m_name << " learned " << spell->getName() << std::endl;
}

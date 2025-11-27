#include "../../include/Player.h"
#include "../../include/Item.h"
#include "../../include/Spell.h"
#ifndef NO_VULKAN
#include "../../include/VulkanRenderer.h"
#endif
#include <algorithm>
#include <iostream>

Player::Player(CharacterClass charClass, const std::string& name)
    : m_characterClass(charClass), m_name(name), m_level(1), m_health(100), m_mana(50),
      m_maxHealth(100), m_maxMana(50), m_strength(10), m_magic(10), m_speed(10), m_defense(10),
      m_x(0.0f), m_y(0.0f) {
    initializeStats();
}

// Default constructor
Player::Player() 
    : m_characterClass(CharacterClass::WARRIOR), m_name("DefaultPlayer"), m_level(1), 
      m_health(100), m_mana(50), m_maxHealth(100), m_maxMana(50), m_strength(10), m_magic(10), m_speed(10), m_defense(10),
      m_x(0.0f), m_y(0.0f) {
    // Initialize with default values
}

Player::~Player() {}

bool Player::initialize() {
    // For now, we'll just print a message
    std::cout << "Initializing player: " << m_name << std::endl;
    return true;
}

void Player::update(float deltaTime) {
    // In a real implementation, we would update player state here
    // For now, we'll just print a message
    // std::cout << "Updating player: " << m_name << std::endl;
}

#ifndef NO_VULKAN
void Player::render(VulkanRenderer* renderer) {
    // Render a simple sprite for the player
    renderer->renderSprite(m_x, m_y, 0.1f, 0.1f);
}
#else
void Player::render() {
    // Empty implementation when Vulkan is disabled
}
#endif

void Player::initializeStats() {
    // Initialize stats based on character class
    switch (m_characterClass) {
        case CharacterClass::MAGE:
            m_magic += 15;
            m_mana = 100;
            m_maxMana = 100;
            m_health = 80;
            m_maxHealth = 80;
            m_defense = 5;
            break;
        case CharacterClass::WARRIOR:
            m_strength += 15;
            m_health = 150;
            m_maxHealth = 150;
            m_mana = 30;
            m_maxMana = 30;
            m_defense = 15;
            break;
        case CharacterClass::ROGUE:
            m_speed += 15;
            m_strength += 5;
            m_health = 90;
            m_maxHealth = 90;
            m_mana = 40;
            m_maxMana = 40;
            m_magic = 7;
            m_defense = 8;
            break;
    }
    
    std::cout << "Created " << m_name << " the ";
    switch (m_characterClass) {
        case CharacterClass::MAGE: std::cout << "Mage"; break;
        case CharacterClass::WARRIOR: std::cout << "Warrior"; break;
        case CharacterClass::ROGUE: std::cout << "Rogue"; break;
    }
    std::cout << " with stats: HP=" << m_health << "/" << m_maxHealth << ", MP=" << m_mana << "/" << m_maxMana
              << ", STR=" << m_strength << ", MAG=" << m_magic 
              << ", SPD=" << m_speed << ", DEF=" << m_defense << std::endl;
}

void Player::takeDamage(int damage) {
    int actualDamage = (std::max)(0, damage - m_defense / 2);
    m_health -= actualDamage;
    m_health = (std::max)(0, m_health);
    
    std::cout << m_name << " took " << actualDamage << " damage. HP: " << m_health << "/" << m_maxHealth << std::endl;
}

void Player::heal(int amount) {
    m_health += amount;
    if (m_health > m_maxHealth) {
        m_health = m_maxHealth;
    }
    
    std::cout << m_name << " healed for " << amount << " HP. HP: " << m_health << "/" << m_maxHealth << std::endl;
}

void Player::addItem(Item* item) {
    m_inventory.push_back(item);
    std::cout << m_name << " picked up " << item->getName() << std::endl;
}

void Player::removeItem(Item* item) {
    auto it = std::find(m_inventory.begin(), m_inventory.end(), item);
    if (it != m_inventory.end()) {
        m_inventory.erase(it);
        std::cout << m_name << " removed " << item->getName() << " from inventory" << std::endl;
    }
}

void Player::addSpell(Spell* spell) {
    if (m_characterClass == CharacterClass::MAGE) {
        m_spells.push_back(spell);
        std::cout << m_name << " learned spell: " << spell->getName() << std::endl;
    } else {
        std::cout << m_name << " cannot learn spells!" << std::endl;
    }
}

bool Player::canCastSpells() const {
    return m_characterClass == CharacterClass::MAGE;
}

bool Player::canSteal() const {
    return m_characterClass == CharacterClass::ROGUE;
}

bool Player::hasCriticalStrike() const {
    return m_characterClass == CharacterClass::ROGUE;
}

void Player::levelUp() {
    m_level++;
    
    // Increase stats based on character class
    switch (m_characterClass) {
        case CharacterClass::MAGE:
            m_maxHealth += 10;
            m_health = m_maxHealth;  // Full heal
            m_maxMana += 20;
            m_mana = m_maxMana;      // Full mana restore
            m_magic += 3;
            m_defense += 1;
            break;
        case CharacterClass::WARRIOR:
            m_maxHealth += 20;
            m_health = m_maxHealth;  // Full heal
            m_maxMana += 5;
            m_mana = m_maxMana;      // Full mana restore
            m_strength += 3;
            m_defense += 2;
            break;
        case CharacterClass::ROGUE:
            m_maxHealth += 15;
            m_health = m_maxHealth;  // Full heal
            m_maxMana += 10;
            m_mana = m_maxMana;      // Full mana restore
            m_speed += 3;
            m_strength += 2;
            m_defense += 1;
            break;
    }
    
    std::cout << m_name << " leveled up to level " << m_level << "!" << std::endl;
}

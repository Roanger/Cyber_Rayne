#include "../../include/Player.h"
#include "../../include/Item.h"
#include "../../include/Ability.h"
#include "../../include/AbilityRegistry.h"
#include "../../include/Map.h"
#ifndef NO_VULKAN
#include "../../include/VulkanRenderer.h"
#endif
#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>

const char* Player::classToString(CharacterClass charClass) {
    switch (charClass) {
        case CharacterClass::NETRUNNER:   return "Netrunner";
        case CharacterClass::SOLO:        return "Solo";
        case CharacterClass::INFILTRATOR: return "Infiltrator";
    }
    return "Unknown";
}

bool Player::classFromString(const std::string& name, CharacterClass& out) {
    for (int i = 0; i < CHARACTER_CLASS_COUNT; ++i) {
        auto candidate = static_cast<CharacterClass>(i);
        if (name == classToString(candidate)) {
            out = candidate;
            return true;
        }
    }
    return false;
}

bool Player::isValidClassOrdinal(int ordinal) {
    return ordinal >= 0 && ordinal < CHARACTER_CLASS_COUNT;
}

Player::Player(CharacterClass charClass, const std::string& name)
    : m_characterClass(charClass), m_name(name), m_level(1), m_health(100), m_maxHealth(100),
      m_mana(50), m_maxMana(50), m_strength(10), m_magic(10), m_speed(10), m_defense(10),
      m_experience(0), m_gold(0),
      m_x(0.0f), m_y(0.0f), m_startX(0.0f), m_startY(0.0f), m_targetX(0.0f), m_targetY(0.0f), 
      m_isMoving(false), m_moveProgress(0.0f), m_moveSpeed(8.0f), m_textureIndex(-1) {
    initializeStats();
    refreshAbilities();
}

// Default constructor
Player::Player() 
    : m_characterClass(CharacterClass::SOLO), m_name("DefaultPlayer"), m_level(1),
      m_health(100), m_maxHealth(100), m_mana(50), m_maxMana(50), m_strength(10), m_magic(10), m_speed(10), m_defense(10),
      m_experience(0), m_gold(0),
      m_x(0.0f), m_y(0.0f), m_startX(0.0f), m_startY(0.0f), m_targetX(0.0f), m_targetY(0.0f),
      m_isMoving(false), m_moveProgress(0.0f), m_moveSpeed(8.0f), m_textureIndex(-1) {
    initializeStats();
    refreshAbilities();
}

Player::~Player() {}

bool Player::consumeStepCompleted() {
    bool completed = m_stepCompleted;
    m_stepCompleted = false;
    return completed;
}

bool Player::initialize() {
    // For now, we'll just print a message
    std::cout << "Initializing player: " << m_name << std::endl;
    return true;
}

namespace {
// Seconds each walk-cycle frame is shown.
constexpr float WALK_FRAME_TIME = 0.11f;
// How long the character may stand between two steps before the walk cycle
// stops. Holding a direction leaves a frame's gap between steps, and without
// this the standing pose would flicker in on every tile.
constexpr float WALK_GRACE = 0.06f;
}  // namespace

int Player::getWalkFrame() const {
    if (m_walkClock < 0.0f) {
        return STANDING_FRAME;
    }
    return static_cast<int>(m_walkClock / WALK_FRAME_TIME) % WALK_FRAMES;
}

void Player::update(float deltaTime) {
    if (m_isMoving) {
        m_walkClock = std::max(0.0f, m_walkClock) + deltaTime;
        m_standingFor = 0.0f;
    } else {
        // Nothing picked up last frame's leftover movement; drop it.
        m_stepOverflow = 0.0f;
        m_standingFor += deltaTime;
        if (m_standingFor > WALK_GRACE) {
            m_walkClock = -1.0f;
        }
    }

    // Handle smooth movement interpolation
    if (m_isMoving) {
        // Increment movement progress based on speed and deltaTime
        m_moveProgress += m_moveSpeed * deltaTime;
        
        if (m_moveProgress >= 1.0f) {
            // Movement complete - snap to target position
            m_x = m_targetX;
            m_y = m_targetY;
            m_isMoving = false;
            // Capped so one very long frame cannot skip most of a tile.
            m_stepOverflow = std::min(m_moveProgress - 1.0f, 0.5f);
            m_moveProgress = 0.0f;
            m_stepCompleted = true;
        } else {
            // Linear interpolation between start and target
            m_x = m_startX + (m_targetX - m_startX) * m_moveProgress;
            m_y = m_startY + (m_targetY - m_startY) * m_moveProgress;
        }
    }
}

#ifndef NO_VULKAN
void Player::loadTexture(VulkanRenderer* renderer) {
    if (!renderer) return;
    
    std::string base = renderer->getAssetsBasePath();

    // Asset filename is the lowercased class name, e.g. "netrunner.png".
    std::string className = classToString(m_characterClass);
    std::transform(className.begin(), className.end(), className.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::string texturePath = base + "/characters/" + className + ".png";


    if (std::filesystem::exists(texturePath)) {
        m_textureIndex = renderer->loadTexture(texturePath);
        std::cout << "Loaded player texture: " << texturePath << " (index: " << m_textureIndex << ")" << std::endl;
    } else {
        std::cerr << "Player texture not found: " << texturePath << std::endl;
        m_textureIndex = -1;
    }
}

void Player::render(VulkanRenderer* renderer) {
    // Render player with texture if available
    if (m_textureIndex >= 0) {
        renderer->renderSpriteWithTexture(m_x, m_y, 0.1f, 0.1f, m_textureIndex);
    } else {
        // Fallback to white sprite if no texture
        renderer->renderSprite(m_x, m_y, 0.1f, 0.1f);
    }
}
#else
void Player::render() {
    // Empty implementation when Vulkan is disabled
}
#endif

void Player::initializeStats() {
    // Initialize stats based on character class
    switch (m_characterClass) {
        case CharacterClass::NETRUNNER:
            m_magic += 15;
            m_mana = 100;
            m_maxMana = 100;
            m_health = 80;
            m_maxHealth = 80;
            m_defense = 5;
            break;
        case CharacterClass::SOLO:
            m_strength += 15;
            m_health = 150;
            m_maxHealth = 150;
            m_mana = 30;
            m_maxMana = 30;
            m_defense = 15;
            break;
        case CharacterClass::INFILTRATOR:
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

    std::cout << "Created " << m_name << " the " << classToString(m_characterClass)
              << " with stats: HP=" << m_health << "/" << m_maxHealth << ", MP=" << m_mana << "/" << m_maxMana
              << ", STR=" << m_strength << ", MAG=" << m_magic 
              << ", SPD=" << m_speed << ", DEF=" << m_defense << std::endl;
}

void Player::takeDamage(int damage) {
    int actualDamage = computeMitigatedDamage(damage, getEffectiveDefense());
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

void Player::refreshAbilities() {
    for (const Ability* ability : AbilityRegistry::unlockedFor(m_characterClass, m_level)) {
        if (!knowsAbility(ability)) {
            learnAbility(ability);
            std::cout << m_name << " learned " << ability->getName() << "!" << std::endl;
        }
    }
}

void Player::levelUp() {
    m_level++;
    
    // Increase stats based on character class
    switch (m_characterClass) {
        case CharacterClass::NETRUNNER:
            m_maxHealth += 10;
            m_health = m_maxHealth;  // Full heal
            m_maxMana += 20;
            m_mana = m_maxMana;      // Full mana restore
            m_magic += 3;
            m_defense += 1;
            break;
        case CharacterClass::SOLO:
            m_maxHealth += 20;
            m_health = m_maxHealth;  // Full heal
            m_maxMana += 5;
            m_mana = m_maxMana;      // Full mana restore
            m_strength += 3;
            m_defense += 2;
            break;
        case CharacterClass::INFILTRATOR:
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
    refreshAbilities();
}

void Player::move(float dx, float dy, Map* map) {
    if (!map) return;
    
    // Don't allow new movement if already moving
    if (m_isMoving) return;

    // Turn to face the way that was pressed, even if the way is blocked.
    face(static_cast<int>(dx), static_cast<int>(dy));
    
    // Calculate target tile position
    int currentTileX = static_cast<int>(m_x);
    int currentTileY = static_cast<int>(m_y);
    int targetTileX = currentTileX + static_cast<int>(dx);
    int targetTileY = currentTileY + static_cast<int>(dy);
    
    // Check if the target tile is walkable
    if (map->isTileWalkable(targetTileX, targetTileY)) {
        // Start smooth movement
        m_startX = m_x;
        m_startY = m_y;
        m_targetX = static_cast<float>(targetTileX);
        m_targetY = static_cast<float>(targetTileY);
        m_isMoving = true;
        m_moveProgress = m_stepOverflow;
        m_stepOverflow = 0.0f;
        // Show the head start now rather than a frame late.
        m_x = m_startX + (m_targetX - m_startX) * m_moveProgress;
        m_y = m_startY + (m_targetY - m_startY) * m_moveProgress;
    }
}

void Player::face(int dx, int dy) {
    if (m_isMoving) return;
    if (dx < 0) m_facing = Facing::LEFT;
    else if (dx > 0) m_facing = Facing::RIGHT;
    else if (dy < 0) m_facing = Facing::UP;
    else if (dy > 0) m_facing = Facing::DOWN;
}

void Player::teleport(float x, float y) {
    m_x = x;
    m_y = y;
    m_startX = x;
    m_startY = y;
    m_targetX = x;
    m_targetY = y;
    m_isMoving = false;
    m_moveProgress = 0.0f;
    m_stepCompleted = false;
    m_stepOverflow = 0.0f;
    m_walkClock = -1.0f;
}

void Player::moveUp(Map* map) {
    move(0.0f, -1.0f, map);
}

void Player::moveDown(Map* map) {
    move(0.0f, 1.0f, map);
}

void Player::moveLeft(Map* map) {
    move(-1.0f, 0.0f, map);
}

void Player::moveRight(Map* map) {
    move(1.0f, 0.0f, map);
}

int Player::getExperienceToNextLevel() const {
    // XP curve: 100 * level^1.5, rounded.
    return static_cast<int>(100.0 * std::pow(static_cast<float>(m_level), 1.5f));
}

void Player::addExperience(int exp) {
    if (exp <= 0) return;
    m_experience += exp;
    std::cout << m_name << " gained " << exp << " XP. Total: " << m_experience
              << "/" << getExperienceToNextLevel() << std::endl;
    while (m_experience >= getExperienceToNextLevel()) {
        m_experience -= getExperienceToNextLevel();
        levelUp();
    }
}

bool Player::spendGold(int amount) {
    if (m_gold < amount) {
        std::cout << m_name << " doesn't have enough gold (" << m_gold << "/" << amount << ")" << std::endl;
        return false;
    }
    m_gold -= amount;
    return true;
}

int Player::getEquipmentAttackBonus() const {
    return m_equippedWeapon ? m_equippedWeapon->getAttackBonus() : 0;
}

int Player::getEquipmentDefenseBonus() const {
    return m_equippedArmor ? m_equippedArmor->getDefenseBonus() : 0;
}

void Player::equipWeapon(std::unique_ptr<Item> weapon) {
    if (!weapon) return;
    if (weapon->getType() != Item::ItemType::WEAPON) {
        std::cout << weapon->getName() << " is not a weapon!" << std::endl;
        return;
    }
    m_equippedWeapon = std::move(weapon);
    std::cout << m_name << " equipped " << m_equippedWeapon->getName() << std::endl;
}

void Player::equipArmor(std::unique_ptr<Item> armor) {
    if (!armor) return;
    if (armor->getType() != Item::ItemType::ARMOR) {
        std::cout << armor->getName() << " is not armor!" << std::endl;
        return;
    }
    m_equippedArmor = std::move(armor);
    std::cout << m_name << " equipped " << m_equippedArmor->getName() << std::endl;
}

std::unique_ptr<Item> Player::unequipWeapon() {
    return std::move(m_equippedWeapon);
}

std::unique_ptr<Item> Player::unequipArmor() {
    return std::move(m_equippedArmor);
}

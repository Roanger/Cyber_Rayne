#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Combatant.h"

#ifndef NO_VULKAN
class VulkanRenderer;
#endif

class Item;

class Player : public Combatant {
public:
    // Ordinal order is part of the save format: SaveSystem persists this as a
    // bare int. Renaming enumerators is safe; reordering or inserting silently
    // reinterprets existing saves.
    enum class CharacterClass {
        NETRUNNER,   // 0 — tech caster
        SOLO,        // 1 — physical frontline
        INFILTRATOR  // 2 — speed and attrition
    };

    static constexpr int CHARACTER_CLASS_COUNT = 3;

    // Human-readable class name, e.g. "Netrunner". Also the basis for asset
    // filenames (lowercased).
    static const char* classToString(CharacterClass charClass);
    // Returns false if the name matches no class; out is left untouched.
    static bool classFromString(const std::string& name, CharacterClass& out);
    // True if the raw int is a valid CharacterClass ordinal.
    static bool isValidClassOrdinal(int ordinal);

    // Constructor with character class and name
    Player(CharacterClass charClass, const std::string& name);
    
    // Default constructor
    Player();
    
    ~Player();

    bool initialize();
    void update(float deltaTime);
#ifndef NO_VULKAN
    void loadTexture(class VulkanRenderer* renderer);
    void render(class VulkanRenderer* renderer);
#else
    void render();
#endif

    // Combatant interface
    const std::string& getName() const override { return m_name; }
    Allegiance getAllegiance() const override { return Allegiance::PLAYER; }
    int getLevel() const override { return m_level; }
    int getHealth() const override { return m_health; }
    int getMaxHealth() const override { return m_maxHealth; }
    int getMana() const override { return m_mana; }
    int getMaxMana() const override { return m_maxMana; }
    int getStrength() const override { return m_strength; }
    int getMagic() const override { return m_magic; }
    int getSpeed() const override { return m_speed; }
    int getDefense() const override { return m_defense; }
    bool isAlive() const override { return m_health > 0; }

    // Player-specific getters
    CharacterClass getCharacterClass() const { return m_characterClass; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    int getTextureIndex() const { return m_textureIndex; }

    // Setters
    void setName(const std::string& name) { m_name = name; }
    void setLevel(int level) { m_level = level; }
    void setHealth(int health) override { m_health = health; }
    void setMana(int mana) override { m_mana = mana; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }
    // Place the player on a tile and cancel any step in progress. Use this for
    // map transitions: setPosition alone leaves the interpolation running, and
    // the next update() drags the player back toward the old target.
    void teleport(float x, float y);
    bool isMoving() const { return m_isMoving; }
    
    // True once per completed tile-to-tile move, cleared on read. Drives
    // step-based random encounters.
    bool consumeStepCompleted();

    // Movement
    void move(float dx, float dy, class Map* map);
    void moveUp(class Map* map);
    void moveDown(class Map* map);
    void moveLeft(class Map* map);
    void moveRight(class Map* map);

    // Combat functions
    void takeDamage(int damage) override;
    void heal(int amount) override;

    // Grant every ability this class has unlocked at its current level.
    // Idempotent, so it is safe to call after loading a save.
    void refreshAbilities();

    // Experience and gold
    int getExperience() const { return m_experience; }
    int getExperienceToNextLevel() const;
    void addExperience(int exp);
    int getGold() const { return m_gold; }
    void addGold(int amount) { m_gold += amount; }
    bool spendGold(int amount);

    // Equipment
    int getEquipmentAttackBonus() const override;
    int getEquipmentDefenseBonus() const override;
    const Item* getEquippedWeapon() const { return m_equippedWeapon.get(); }
    const Item* getEquippedArmor() const { return m_equippedArmor.get(); }
    void equipWeapon(std::unique_ptr<Item> weapon);
    void equipArmor(std::unique_ptr<Item> armor);
    std::unique_ptr<Item> unequipWeapon();
    std::unique_ptr<Item> unequipArmor();

    // Level up function
    void levelUp();

private:
    // Initialize stats based on character class
    void initializeStats();

    CharacterClass m_characterClass;
    std::string m_name;
    int m_level;
    int m_health;
    int m_maxHealth;
    int m_mana;
    int m_maxMana;
    int m_strength;
    int m_magic;
    int m_speed;
    int m_defense;
    int m_experience;
    int m_gold;
    float m_x;
    float m_y;
    
    // Movement state for smooth interpolation
    float m_startX;
    float m_startY;
    float m_targetX;
    float m_targetY;
    bool m_isMoving;
    bool m_stepCompleted = false;
    float m_moveProgress;
    float m_moveSpeed; // tiles per second
    
    int m_textureIndex;

    std::unique_ptr<Item> m_equippedWeapon;
    std::unique_ptr<Item> m_equippedArmor;
};

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Combatant.h"

#ifndef NO_VULKAN
class VulkanRenderer;
#endif

class Item;
class Spell;

class Player : public Combatant {
public:
    enum class CharacterClass {
        MAGE,
        WARRIOR,
        ROGUE
    };

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
    
    // Movement
    void move(float dx, float dy, class Map* map);
    void moveUp(class Map* map);
    void moveDown(class Map* map);
    void moveLeft(class Map* map);
    void moveRight(class Map* map);

    // Combat functions
    void takeDamage(int damage) override;
    void heal(int amount) override;

    // Inventory management
    void addItem(std::unique_ptr<Item> item);
    void removeItem(const Item* item);
    const std::vector<std::unique_ptr<Item>>& getInventory() const { return m_inventory; }

    // Spell management (for Mage)
    void addSpell(std::shared_ptr<Spell> spell);
    const std::vector<std::shared_ptr<Spell>>& getSpells() const { return m_spells; }

    // Character class specific abilities
    bool canCastSpells() const;
    bool canSteal() const;
    bool hasCriticalStrike() const;

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
    float m_x;
    float m_y;
    
    // Movement state for smooth interpolation
    float m_startX;
    float m_startY;
    float m_targetX;
    float m_targetY;
    bool m_isMoving;
    float m_moveProgress;
    float m_moveSpeed; // tiles per second
    
    int m_textureIndex;

    std::vector<std::unique_ptr<Item>> m_inventory;
    std::vector<std::shared_ptr<Spell>> m_spells;
};

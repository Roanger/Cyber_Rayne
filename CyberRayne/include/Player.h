#pragma once

#include <string>
#include <vector>
#include <memory>

#ifndef NO_VULKAN
class VulkanRenderer;
#endif

class Item;
class Spell;

class Player {
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

    // Getters
    const std::string& getName() const { return m_name; }
    int getLevel() const { return m_level; }
    int getHealth() const { return m_health; }
    int getMana() const { return m_mana; }
    CharacterClass getCharacterClass() const { return m_characterClass; }
    int getStrength() const { return m_strength; }
    int getMagic() const { return m_magic; }
    int getSpeed() const { return m_speed; }
    int getDefense() const { return m_defense; }
    int getMaxHealth() const { return m_maxHealth; }
    int getMaxMana() const { return m_maxMana; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }

    // Setters
    void setName(const std::string& name) { m_name = name; }
    void setLevel(int level) { m_level = level; }
    void setHealth(int health) { m_health = health; }
    void setMana(int mana) { m_mana = mana; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }

    // Combat functions
    void takeDamage(int damage);
    bool isAlive() const { return m_health > 0; }
    void heal(int amount);

    // Inventory management
    void addItem(Item* item);
    void removeItem(Item* item);
    const std::vector<Item*>& getInventory() const { return m_inventory; }

    // Spell management (for Mage)
    void addSpell(Spell* spell);
    const std::vector<Spell*>& getSpells() const { return m_spells; }

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
    
    int m_textureIndex;

    std::vector<Item*> m_inventory;
    std::vector<Spell*> m_spells;
};

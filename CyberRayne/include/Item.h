#pragma once

#include <string>

class Player;

class Item {
public:
    enum class ItemType {
        WEAPON,
        ARMOR,
        CONSUMABLE,
        KEY_ITEM,
        MATERIAL
    };

    Item(const std::string& name, ItemType type, const std::string& description);
    virtual ~Item();

    // Getters
    const std::string& getName() const { return m_name; }
    ItemType getType() const { return m_type; }
    const std::string& getDescription() const { return m_description; }
    int getValue() const { return m_value; }

    // Setters
    void setValue(int value) { m_value = value; }

    // Item actions
    virtual void use(Player* player) = 0;

protected:
    std::string m_name;
    ItemType m_type;
    std::string m_description;
    int m_value;
};

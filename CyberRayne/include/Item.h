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
    // Catalog ID, set by ItemCatalog::create. Empty for ad-hoc items.
    const std::string& getId() const { return m_id; }
    void setId(const std::string& id) { m_id = id; }

    // Setters
    void setValue(int value) { m_value = value; }

    // Combat bonuses granted while equipped. Declared on the base so callers
    // holding an Item* can read them without a downcast.
    virtual int getAttackBonus() const { return 0; }
    virtual int getDefenseBonus() const { return 0; }

    // Item actions
    virtual void use(Player* player) = 0;
    // Whether using this on the target would do anything. Checked before an
    // item is consumed, so nothing is wasted. Equipment is never "used".
    virtual bool canUseOn(const Player* target) const { (void)target; return false; }

protected:
    std::string m_name;
    ItemType m_type;
    std::string m_description;
    int m_value;
    std::string m_id;
};

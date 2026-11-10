#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Item.h"

// Every item in the game, by stable string ID. IDs are what saves store, so
// renaming one orphans it in existing saves.
namespace ItemCatalog {

// A fresh instance of the item, or nullptr for an unknown ID.
std::unique_ptr<Item> create(const std::string& id);
bool exists(const std::string& id);

}  // namespace ItemCatalog

// The party's shared bag: stacks of items keyed by catalog ID.
// Gameplay logic only — no rendering dependencies.
class Inventory {
public:
    struct Stack {
        std::string id;
        int count = 0;
        // One instance used for the name, description, type, and effect.
        std::unique_ptr<Item> prototype;
    };

    // Returns false for an unknown ID or a count below 1.
    bool add(const std::string& id, int count = 1);
    // Returns false, removing nothing, if the bag holds fewer than count.
    bool remove(const std::string& id, int count = 1);
    int count(const std::string& id) const;

    const Stack* find(const std::string& id) const;
    // Stacks in the order they were first picked up.
    const std::vector<Stack>& getStacks() const { return m_stacks; }
    std::vector<const Stack*> getStacksOfType(Item::ItemType type) const;

    bool isEmpty() const { return m_stacks.empty(); }
    void clear() { m_stacks.clear(); }

private:
    std::vector<Stack> m_stacks;
};

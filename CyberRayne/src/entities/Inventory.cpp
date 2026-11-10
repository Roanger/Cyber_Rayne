#include "../../include/Inventory.h"
#include "../../include/Equipment.h"

#include <algorithm>

namespace ItemCatalog {

std::unique_ptr<Item> create(const std::string& id) {
    std::unique_ptr<Item> item;

    // Consumables
    if (id == "stim_pack") {
        item = std::make_unique<HealingItem>("Stim Pack", "Restores 40 HP to one ally.", 40);
    } else if (id == "trauma_kit") {
        item = std::make_unique<HealingItem>("Trauma Kit", "Restores 120 HP to one ally.", 120);
    } else if (id == "defib_patch") {
        item = std::make_unique<ReviveItem>("Defib Patch", "Gets a downed ally back up at a quarter health.", 25);
    }
    // Weapons
    else if (id == "shock_baton") {
        item = std::make_unique<Weapon>("Shock Baton", "Street-legal, barely. ATK +4.", 4);
    } else if (id == "mono_katana") {
        item = std::make_unique<Weapon>("Mono-Katana", "A monomolecular edge. ATK +8.", 8);
    }
    // Armor
    else if (id == "street_jacket") {
        item = std::make_unique<Armor>("Street Jacket", "Reinforced synth-leather. DEF +3.", 3);
    } else if (id == "kevlar_weave") {
        item = std::make_unique<Armor>("Kevlar Weave", "Corporate-grade layered plate. DEF +6.", 6);
    }

    if (item) {
        item->setId(id);
    }
    return item;
}

bool exists(const std::string& id) {
    return create(id) != nullptr;
}

}  // namespace ItemCatalog

bool Inventory::add(const std::string& id, int count) {
    if (count < 1) {
        return false;
    }
    for (auto& stack : m_stacks) {
        if (stack.id == id) {
            stack.count += count;
            return true;
        }
    }

    auto prototype = ItemCatalog::create(id);
    if (!prototype) {
        return false;
    }
    Stack stack;
    stack.id = id;
    stack.count = count;
    stack.prototype = std::move(prototype);
    m_stacks.push_back(std::move(stack));
    return true;
}

bool Inventory::remove(const std::string& id, int count) {
    if (count < 1) {
        return false;
    }
    auto it = std::find_if(m_stacks.begin(), m_stacks.end(),
                           [&id](const Stack& stack) { return stack.id == id; });
    if (it == m_stacks.end() || it->count < count) {
        return false;
    }
    it->count -= count;
    if (it->count == 0) {
        m_stacks.erase(it);
    }
    return true;
}

int Inventory::count(const std::string& id) const {
    const Stack* stack = find(id);
    return stack ? stack->count : 0;
}

const Inventory::Stack* Inventory::find(const std::string& id) const {
    for (const auto& stack : m_stacks) {
        if (stack.id == id) {
            return &stack;
        }
    }
    return nullptr;
}

std::vector<const Inventory::Stack*> Inventory::getStacksOfType(Item::ItemType type) const {
    std::vector<const Stack*> result;
    for (const auto& stack : m_stacks) {
        if (stack.prototype && stack.prototype->getType() == type) {
            result.push_back(&stack);
        }
    }
    return result;
}
